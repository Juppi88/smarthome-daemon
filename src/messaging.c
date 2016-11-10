#include "messaging.h"
#include "config.h"
#include "logger.h"
#include "main.h"
#include "utils.h"
#include "MQTTAsync.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define CLIENTID "smarthome_daemon"
#define QOS 1

static char mqtt_server[128];
static uint16_t mqtt_port = 1883;

static MQTTAsync client;
static bool is_connected = false;

struct mqtt_subscription_t {
	char *topic;
	void* context;
	message_update_t callback;
	struct mqtt_subscription_t *next;
};

struct mqtt_subscription_t *subscriptions = NULL;

// --------------------------------------------------------------------------------

static void messaging_connect(void);
static void messaging_disconnect(void);
static struct mqtt_subscription_t *messaging_get_subscription(const char *topic, void *context, message_update_t callback);
static void messaging_register_subscription(struct mqtt_subscription_t *sub);
static void messaging_unregister_subscription(struct mqtt_subscription_t *sub);
static void messaging_on_connect_success(void *context, MQTTAsync_successData *response);
static void messaging_on_connect_failure(void *context, MQTTAsync_failureData *response);
static void messaging_on_connection_lost(void *context, char *cause);
static int messaging_on_message_arrived(void *context, char *topic, int topic_len, MQTTAsync_message *message);
static void messaging_on_message_delivered(void *context, MQTTAsync_token token);

CONFIG_HANDLER(set_mqtt_server);
CONFIG_HANDLER(set_mqtt_port);

// --------------------------------------------------------------------------------

void messaging_initialize(void)
{
	// Register config setters for the MQTT server settings.
	config_add_command_handler("mqtt_server", set_mqtt_server);
	config_add_command_handler("mqtt_port", set_mqtt_port);

	// Load the MQTT client config file.
	char mqtt_config[260];
	snprintf(mqtt_config, sizeof(mqtt_config), "%s/mqtt.conf", get_config_directory());
	config_parse_file("./config/mqtt.conf");

	// If all the necessary settings are available, create an MQTT client and connect to the server.
	if (*mqtt_server != 0) {
		messaging_connect();
	}
}

void messaging_shutdown(void)
{
	if (is_connected) {
		messaging_disconnect();
	}

	// Destroy all subscriptions.
	LIST_FOREACH_SAFE(struct mqtt_subscription_t, sub, tmp, subscriptions) {
		utils_free(sub->topic);
		utils_free(sub);
	}

	subscriptions = NULL;
}

void messaging_publish(const char *message, const char *topic_fmt, ...)
{
	if (!is_connected) {
		return;
	}

	char topic[256];
	va_list args;

	va_start(args, topic_fmt);
	vsnprintf(topic, sizeof(topic), topic_fmt, args);
	va_end(args);

	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	opts.context = client;

	MQTTAsync_message msg = MQTTAsync_message_initializer;
	msg.payload = (void *)message;
	msg.payloadlen = (int)strlen(message);
	msg.qos = QOS;
	msg.retained = 0;

	MQTTAsync_sendMessage(client, topic, &msg, &opts);
}

void messaging_subscribe(void *context, message_update_t callback, const char *topic_fmt, ...)
{
	char topic[256];
	va_list args;

	va_start(args, topic_fmt);
	vsnprintf(topic, sizeof(topic), topic_fmt, args);
	va_end(args);

	// Add to the list of subscriptions and then actually subscribe if connected.
	// If the subscription has been added already, don't do anything.
	struct mqtt_subscription_t *sub = messaging_get_subscription(topic, context, callback);

	if (sub != NULL) {
		return;
	}

	sub = utils_alloc(sizeof(*sub));
	sub->topic = utils_duplicate_string(topic);
	sub->context = context;
	sub->callback = callback;

	LIST_ADD_ENTRY(subscriptions, sub);

	if (is_connected) {
		messaging_register_subscription(sub);
	}
}

void messaging_unsubscribe(void *context, message_update_t callback, const char *topic_fmt, ...)
{
	char topic[256];
	va_list args;

	va_start(args, topic_fmt);
	vsnprintf(topic, sizeof(topic), topic_fmt, args);
	va_end(args);

	struct mqtt_subscription_t *sub = messaging_get_subscription(topic, context, callback);

	if (sub == NULL) {
		return;
	}

	// Actually unsubscribe if connected, then remove from the list and destroy.
	if (is_connected) {
		messaging_unregister_subscription(sub);
	}

	LIST_REMOVE_ENTRY(struct mqtt_subscription_t, sub, subscriptions);

	utils_free(sub->topic);
	utils_free(sub);
}

static void messaging_connect(void)
{
	if (is_connected) {
		return;
	}

	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	//MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;

	char address[160];
	snprintf(address, sizeof(address), "tcp://%s:%u", mqtt_server, mqtt_port);

	output_log("Connecting to MQTT server %s...", address);

	MQTTAsync_create(&client, address, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	MQTTAsync_setCallbacks(client, NULL, messaging_on_connection_lost, messaging_on_message_arrived, messaging_on_message_delivered);

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = messaging_on_connect_success;
	conn_opts.onFailure = messaging_on_connect_failure;
	conn_opts.context = client;

	MQTTAsync_connect(client, &conn_opts);
}

static void messaging_disconnect(void)
{
	if (!is_connected) {
		return;
	}

	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	opts.context = client;

	MQTTAsync_disconnect(client, &opts);
	MQTTAsync_destroy(&client);
	
	is_connected = false;
}

static struct mqtt_subscription_t *messaging_get_subscription(const char *topic, void *context, message_update_t callback)
{
	LIST_FOREACH(struct mqtt_subscription_t, sub, subscriptions) {
		if (strcmp(topic, sub->topic) == 0 &&
			sub->context == context &&
			sub->callback == callback) {
			return sub;
		}
	}

	return NULL;
}

static void messaging_register_subscription(struct mqtt_subscription_t *sub)
{
	if (!is_connected || sub == NULL) {
		return;
	}

	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	opts.onSuccess = NULL;
	opts.onFailure = NULL;
	opts.context = client;

	MQTTAsync_subscribe(client, sub->topic, QOS, &opts);
}

static void messaging_unregister_subscription(struct mqtt_subscription_t *sub)
{
	if (!is_connected || sub == NULL) {
		return;
	}

	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	opts.onSuccess = NULL;
	opts.onFailure = NULL;
	opts.context = client;

	MQTTAsync_unsubscribe(client, sub->topic, &opts);
}

static void messaging_on_connect_success(void *context, MQTTAsync_successData *response)
{
	is_connected = true;

	output_log("Connected to MQTT server.");

	// Register all existing subscriptions on connect.
	LIST_FOREACH(struct mqtt_subscription_t, sub, subscriptions) {
		messaging_register_subscription(sub);
	}
}

static void messaging_on_connect_failure(void *context, MQTTAsync_failureData *response)
{
	is_connected = false;

	output_log("Could not connect to MQTT server!");
}

static void messaging_on_connection_lost(void *context, char *cause)
{
	is_connected = false;

	// It seems we have lost connection to the MQTT server for whatever reason, let's try again.
	output_log("Lost connection to MQTT server. Attempting to reconnect.");
	MQTTAsync_destroy(&client);

	messaging_connect();
}

static int messaging_on_message_arrived(void *context, char *topic, int topic_len, MQTTAsync_message *message)
{
	// Notify all listeners about the updated topic.
	LIST_FOREACH(struct mqtt_subscription_t, sub, subscriptions) {
		if (strcmp(sub->topic, topic) == 0 && sub->callback != NULL) {
			sub->callback(topic, (const char *)message->payload, sub->context);
		}
	}

	// Free the message data.
	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topic);

	return 1;
}

static void messaging_on_message_delivered(void *context, MQTTAsync_token token)
{
}

CONFIG_HANDLER(set_mqtt_server)
{
	if (*args == 0) {
		output_log("Usage: mqtt_server <ip address>");
		return;
	}

	snprintf(mqtt_server, sizeof(mqtt_server), "%s", args);
}

CONFIG_HANDLER(set_mqtt_port)
{
	if (*args == 0) {
		output_log("Usage: mqtt_port <port>");
		return;
	}

	mqtt_port = (uint16_t)atoi(args);
}
