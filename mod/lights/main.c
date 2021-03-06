#include "main.h"
#include "manager.h"
#include "lights_api.h"
#include <string.h>

struct module_import_t api;
struct module_export_t export;
struct alarm_api_t *mod_alarm;
static struct lights_api_t lights_api;

MODULE_API void on_module_loaded(const char *mod);
MODULE_API void on_module_unloaded(const char *mod);

// --------------------------------------------------------------------------------

static void module_process(void)
{
	lights_process();
}

static void module_shutdown(void)
{
	lights_shutdown();
}

MODULE_API struct module_export_t *module_initialize(struct module_import_t *import)
{
	// Store the smarthome API object.
	api = *import;

	// Initialize an API for this module which the other modules (such as the alarm system) can use.
	lights_api.set_min_brightness = lights_set_min_brightness;

	// Initialize the export object and return it.
	export.api_version = MODULE_API_VERSION;
	export.api = &lights_api;
	export.process = module_process;
	export.shutdown = module_shutdown;
	export.on_module_loaded = on_module_loaded;
	export.on_module_unloaded = on_module_unloaded;

	// Initialize the light manager and load all the lights from config files.
	lights_initialize();

	return &export;
}

MODULE_API void on_module_loaded(const char *mod)
{
	// If the alarm module is loaded, request its API pointer.
	if (strcmp(mod, "alarm") == 0) {
		mod_alarm = api.get_module_api(mod);
	}
}

MODULE_API void on_module_unloaded(const char *mod)
{
	// If the alarm module is unloaded, invalidate its API pointer.
	if (strcmp(mod, "alarm") == 0) {
		mod_alarm = NULL;
	}
}
