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

struct module_import_t {
	// Logging
	void (*log_write)(const char *format, ...);
	void (*log_write_error)(const char *format, ...);

	// Messaging
	void (*message_publish)(const char *topic, const char *message);
	void (*message_subscribe)(const char *topic, void *context, message_update_t callback);
	void (*message_unsubscribe)(const char *topic, void *context, message_update_t callback);
};

struct module_export_t {
	uint32_t api_version;
	void (*process)(void);
	void (*shutdown)(void);
};

typedef struct module_export_t *(*module_initialize_t)(struct module_import_t *api);

#endif
