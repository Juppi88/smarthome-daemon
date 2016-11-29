#include "main.h"
#include "alarm.h"
#include <string.h>

struct module_import_t api;
struct module_export_t export;
struct lights_api_t *mod_lights;

MODULE_API void on_module_loaded(const char *mod);
MODULE_API void on_module_unloaded(const char *mod);

// --------------------------------------------------------------------------------

MODULE_API struct module_export_t *module_initialize(struct module_import_t *import)
{
	// Store the smarthome API object.
	api = *import;

	// Initialize the export object and return it.
	export.api_version = MODULE_API_VERSION;
	export.process = alarm_process;
	export.shutdown = alarm_shutdown;
	export.on_module_loaded = on_module_loaded;
	export.on_module_unloaded = on_module_unloaded;

	// Setup this module.
	alarm_initialize();

	return &export;
}

MODULE_API void on_module_loaded(const char *mod)
{
	// If the lights module is loaded, request its API pointer.
	if (strcmp(mod, "lights") == 0) {
		mod_lights = api.get_module_api(mod);
	}
}

MODULE_API void on_module_unloaded(const char *mod)
{
	// If the lights module is unloaded, invalidate its API pointer.
	if (strcmp(mod, "lights") == 0) {
		mod_lights = NULL;
	}
}
