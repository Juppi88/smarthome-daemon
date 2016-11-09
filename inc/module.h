#pragma once
#ifndef __SMARTHOME_MODULE_H
#define __SMARTHOME_MODULE_H

#include "defines.h"

#ifdef _WIN32
#define MODULE_API __declspec(dllexport)
#else
#define MODULE_API
#endif

#define MODULE_API_VERSION 1

typedef void (*message_update_t)(const char *topic, const char *message, void *context);
#define MESSAGE_HANDLER(x) static void x(const char *topic, const char *message, void *context)

typedef void (*config_handler_t)(char *args);
#define CONFIG_HANDLER(x) static void x(char *args)

typedef bool (*web_api_handler_t)(const char *request_url, const char **content);
#define WEB_API_HANDLER(x) static bool x(const char *request_url, const char **content)

struct module_import_t {

	// Config files
	const char *(*get_config_directory)(void);
	void (*config_add_command_handler)(const char *command, config_handler_t method);
	void (*config_parse_file)(const char *path);

	// Logging
	void (*log_write)(const char *format, ...);
	void (*log_write_error)(const char *format, ...);

	// Messaging
	void (*message_publish)(const char *message, const char *topic_fmt, ...);
	void (*message_subscribe)(void *context, message_update_t callback, const char *topic_fmt, ...);
	void (*message_unsubscribe)(void *context, message_update_t callback, const char *topic_fmt, ...);

	// Wep API
	void (*webapi_register_interface)(const char *iface, web_api_handler_t handler);
	void (*webapi_unregister_interface)(const char *iface);

	// Utilities
	void *(*alloc)(size_t size);
	void (*free)(void *ptr);
	char *(*duplicate_string)(const char *text);
};

struct module_export_t {
	uint32_t api_version;
	void (*process)(void);
	void (*shutdown)(void);
};

typedef struct module_export_t *(*module_initialize_t)(struct module_import_t *api);

#endif
