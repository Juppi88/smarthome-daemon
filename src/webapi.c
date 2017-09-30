#include "webapi.h"
#include "config.h"
#include "utils.h"
#include "logger.h"
#include "httpserver.h"
#include <string.h>
#include <stdlib.h>

static uint16_t webapi_port = 8080;
static bool listening = false;

struct web_api_interface_t {
	char *name;
	web_api_handler_t handler;
	struct web_api_interface_t *next;
};

struct web_api_interface_t *interfaces = NULL;

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
		if (http_server_initialize(webapi_port, webapi_handle_request)) {
			listening = true;
			output_log("Started web API server on port %u", webapi_port);
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

	// Try to find the handler for the requested interface.
	struct web_api_interface_t *interface = webapi_get_interface(interface_name);

	if (interface != NULL) {
		// Interface was found, call the handler and let the HTTP server know whether the request was valid.
		const char *content = NULL;

		if (interface->handler(request->request, &content)) {
			response.message = HTTP_200_OK;
			
			// The response also contains some data in JSON format.
			if (content != NULL) {
				response.content = content;
				response.content_type = "text/json";
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

	http_server_add_static_directory("/", args);
}
