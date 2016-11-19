#include "light.h"
#include <stdio.h>
#include <stdlib.h>

static struct light_t *current_light;

#define LIGHT_TOGGLE_TOPIC "home/lights/%s/toggle"
#define LIGHT_MAX_BRIGHTNESS_TOPIC "home/lights/%s/max_brightness"
#define LIGHT_TRANSITION_TIME_TOPIC "home/lights/%s/transition"

// --------------------------------------------------------------------------------

CONFIG_HANDLER(set_light_identifier);
CONFIG_HANDLER(set_light_name);
CONFIG_HANDLER(set_light_enabled);
CONFIG_HANDLER(set_light_pwm_size);

MESSAGE_HANDLER(update_light_toggle);
MESSAGE_HANDLER(update_light_max_brightness);
MESSAGE_HANDLER(update_light_transition_time);

// --------------------------------------------------------------------------------

struct light_t *light_create(const char *config_file)
{
	current_light = api.alloc(sizeof(*current_light));

	// Set default values.
	current_light->is_enabled = true;
	current_light->pwm_size = DEFAULT_PWM_SIZE;
	current_light->transition_time = DEFAULT_TRANSITION_TIME;

	// Set listeners for the config keys and parse the config file.
	api.config_add_command_handler("light_id", set_light_identifier);
	api.config_add_command_handler("light_name", set_light_name);
	api.config_add_command_handler("light_enabled", set_light_enabled);
	api.config_add_command_handler("light_pwm_size", set_light_pwm_size);
	
	api.config_parse_file(config_file);

	// Make sure the config file contained all the necessary info.
	if (current_light->identifier == NULL ||
		current_light->name == NULL) {

		light_destroy(current_light);
		return NULL;
	}

	// Default to maximum brightness.
	current_light->max_brightness = MAX_BRIGHTNESS(current_light->pwm_size);
	
	// Register listeners for the messages regarding the status of the light.
	// We should be receiving the current states as soon as messaging is initialized.
	api.message_subscribe(current_light, update_light_toggle, LIGHT_TOGGLE_TOPIC, current_light->identifier);
	api.message_subscribe(current_light, update_light_max_brightness, LIGHT_MAX_BRIGHTNESS_TOPIC, current_light->identifier);
	api.message_subscribe(current_light, update_light_transition_time, LIGHT_TRANSITION_TIME_TOPIC, current_light->identifier);

	current_light->has_subscribed = true;

	return current_light;
}

void light_destroy(struct light_t *light)
{
	// Unregister all message listeners.
	if (light->has_subscribed) {
		api.message_unsubscribe(light, update_light_toggle, LIGHT_TOGGLE_TOPIC, light->identifier);
		api.message_unsubscribe(light, update_light_max_brightness, LIGHT_MAX_BRIGHTNESS_TOPIC, light->identifier);
		api.message_unsubscribe(light, update_light_transition_time, LIGHT_TRANSITION_TIME_TOPIC, light->identifier);
	}

	api.free(light->identifier);
	api.free(light->name);
	api.free(light);
}

void light_set_toggled(struct light_t *light, bool toggle)
{
	if (light == NULL || light->is_toggled == toggle) {
		return;
	}

	// Send a message to toggle the status of the light.
	// We'll update the light struct once the MQTT server confirms the value has been changed.
	api.message_publish(toggle ? "1" : "0", LIGHT_TOGGLE_TOPIC, light->identifier);
}

void light_set_max_brightness(struct light_t *light, uint16_t max_brightness)
{
	if (light == NULL || light->max_brightness == max_brightness) {
		return;
	}

	char message[8];
	sprintf(message, "%u", max_brightness);

	// Send a message to change the max brightness of the light.
	api.message_publish(message, LIGHT_MAX_BRIGHTNESS_TOPIC, light->identifier);
}

void light_set_transition_time(struct light_t *light, uint16_t transition_time)
{
	if (light == NULL || light->transition_time == transition_time) {
		return;
	}

	char message[8];
	sprintf(message, "%u", transition_time);

	// Send a message to change the transition time of the light.
	api.message_publish(message, LIGHT_TRANSITION_TIME_TOPIC, light->identifier);
}

CONFIG_HANDLER(set_light_identifier)
{
	if (current_light == NULL || args == NULL || *args == 0) {
		return;
	}

	if (current_light->identifier != NULL) {
		api.free(current_light->identifier);
	}

	current_light->identifier = api.duplicate_string(args);
}

CONFIG_HANDLER(set_light_name)
{
	if (current_light == NULL || args == NULL || *args == 0) {
		return;
	}

	if (current_light->name != NULL) {
		api.free(current_light->name);
	}

	current_light->name = api.duplicate_string(args);
}

CONFIG_HANDLER(set_light_enabled)
{
	if (current_light == NULL || args == NULL || *args == 0) {
		return;
	}

	current_light->is_enabled = (atoi(args) != 0);
}

CONFIG_HANDLER(set_light_pwm_size)
{
	if (current_light == NULL || args == NULL || *args == 0) {
		return;
	}

	uint8_t pwm = (uint8_t)atoi(args);
	current_light->pwm_size = (pwm < 32 ? pwm : 31);
}

MESSAGE_HANDLER(update_light_toggle)
{
	struct light_t *light = (struct light_t *)context;
	light->is_toggled = (atoi(message) != 0);
}

MESSAGE_HANDLER(update_light_max_brightness)
{
	struct light_t *light = (struct light_t *)context;
	light->max_brightness = (uint16_t)atoi(message);
}

MESSAGE_HANDLER(update_light_transition_time)
{
	struct light_t *light = (struct light_t *)context;
	light->transition_time = (uint16_t)atoi(message);
}
