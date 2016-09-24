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

struct module_import_t {
	void (*log_write)(const char *format, ...);
	void (*log_write_error)(const char *format, ...);
};

struct module_export_t {
	uint32_t api_version;
	void (*process)(void);
	void (*shutdown)(void);
};

typedef struct module_export_t *(*module_initialize_t)(struct module_import_t *api);

#endif
