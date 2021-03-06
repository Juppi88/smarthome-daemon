#include "webapi.h"
#include "config.h"
#include "utils.h"
#include "logger.h"
#include "httpserver.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static uint16_t webapi_port = 8080;
static char *webapi_static_directory;

static bool listening = false;

struct web_api_interface_t {
	char *name;
	web_api_handler_t handler;
	struct web_api_interface_t *next;
};

struct web_api_interface_t *interfaces = NULL;

// --------------------------------------------------------------------------------

#define JSON_MIME_TYPE "application/json";

#define EMPTY_RESULT_FORMAT \
"{\n"\
"	\"result\": %s\n"\
"}\n"

// --------------------------------------------------------------------------------

static struct web_api_interface_t *webapi_get_interface(const char *name);
static struct http_response_t webapi_handle_request(struct http_request_t *request);

CONFIG_HANDLER(set_webapi_port);
CONFIG_HANDLER(set_webapi_static_directory);

// --------------------------------------------------------------------------------

void webapi_initialize(void)
{
	// Add a config handler for the web server's port.
	config_add_command_handler("webapi_port", set_webapi_port);

	// Set the location of the static HTML files.
	config_add_command_handler("webapi_static_directory", set_webapi_static_directory);
}

void webapi_shutdown(void)
{
	// Shut down the HTTP server.
	if (listening) {
		http_server_shutdown();
		listening = false;
	}

	if (webapi_static_directory != NULL) {
		utils_free(webapi_static_directory);
		webapi_static_directory = NULL;
	}

	// Destroy all interfaces.
	LIST_FOREACH_SAFE(struct web_api_interface_t, iface, tmp, interfaces) {
		utils_free(iface->name);
		utils_free(iface);
	}
}

void webapi_process(void)
{
	if (!listening) {
		// If the HTTP server is not listening yet, initialize it.
		// We do the initialization in the processing loop to give the config a chance to load.
		struct server_settings_t settings;
		memset(&settings, 0, sizeof(settings));

		struct server_directory_t directories[] = { { "/", webapi_static_directory } };

		settings.handler = webapi_handle_request;
		settings.port = webapi_port;
		settings.timeout = 10;
		settings.max_connections = 25;
		settings.connection_timeout = 60;
		settings.directories = directories;
		settings.directories_len = 0;

		// Serve static files from a dedicated folder.
		if (webapi_static_directory != NULL) {
			settings.directories_len = 1;
		}

		if (http_server_initialize(settings)) {

			listening = true;
			output_log("Started web API server on port %u", webapi_port);
		}
		else {
			output_error("Failed to start the web API server on port %u, retrying in 5 seconds...", webapi_port);
			utils_thread_sleep(5000);
		}
	}
	else {
		http_server_listen();
	}
}

void webapi_register_interface(const char *iface, web_api_handler_t handler)
{
	if (iface == NULL || handler == NULL) {
		return;
	}

	// In case an interface with the same name exists already, update the handler method. Otherwise add a new one.
	struct web_api_interface_t *interface = webapi_get_interface(iface);

	if (interface == NULL) {
		interface = utils_alloc(sizeof(*interface));
		interface->name = utils_duplicate_string(iface);
		
		LIST_ADD_ENTRY(interfaces, interface);
	}

	interface->handler = handler;
}

void webapi_unregister_interface(const char *iface)
{
	struct web_api_interface_t *interface = webapi_get_interface(iface);

	if (interface != NULL) {
		LIST_REMOVE_ENTRY(struct web_api_interface_t, interface, interfaces);
	}
}

static struct web_api_interface_t *webapi_get_interface(const char *name)
{
	LIST_FOREACH(struct web_api_interface_t, iface, interfaces) {
		if (strcmp(iface->name, name) == 0) {
			return iface;
		}
	}

	return NULL;
}

static struct http_response_t webapi_handle_request(struct http_request_t *request)
{
	struct http_response_t response;
	response.message = HTTP_400_BAD_REQUEST;
	response.content = NULL;
	response.content_type = NULL;
	response.content_length = 0;
	
	// Find the appropriate handler and call it.
	char interface_name[256], *dst = interface_name;
	register const char *s = request->request;

	// Ignore the leading slash.
	if (*s == '/') {
		++s;
	}

	// Get the interface name from the request URL.
	size_t c = sizeof(interface_name);

	while (*s && --c > 0) {
		if (*s == '/') {
			break;
		}

		*dst++ = *s++;
	}

	*dst = 0;

	char result[32];

	// Try to find the handler for the requested interface.
	struct web_api_interface_t *interface = webapi_get_interface(interface_name);

	if (interface != NULL) {

		// Interface was found, call the handler and let the HTTP server know whether the request was valid.
		const char *content = NULL;

		if (interface->handler(request->request, &content)) {

			response.message = HTTP_200_OK;
			response.content_type = JSON_MIME_TYPE;
			
			// The response also contains some data in JSON format.
			// Every response should at least contain a 'result' bool field.
			if (content != NULL) {
				response.content = content;
			}
			else {
				snprintf(result, sizeof(result), EMPTY_RESULT_FORMAT, "true");
				response.content = result;
			}
		}
	}

	return response;
}

CONFIG_HANDLER(set_webapi_port)
{
	if (*args == 0) {
		output_log("Usage: webapi_port <port>");
		return;
	}

	webapi_port = (uint16_t)atoi(args);
}

CONFIG_HANDLER(set_webapi_static_directory)
{
	if (*args == 0) {
		output_log("Usage: webapi_static_directory <relative path>");
		return;
	}

	if (webapi_static_directory != NULL) {
		utils_free(webapi_static_directory);
	}

	webapi_static_directory = utils_duplicate_string(args);
}
