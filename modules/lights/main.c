#include "main.h"
#include "manager.h"

struct module_import_t api;
struct module_export_t export;

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

	// Initialize the export object and return it.
	export.api_version = MODULE_API_VERSION;
	export.process = module_process;
	export.shutdown = module_shutdown;

	// Initialize the light manager and load all the lights from config files.
	lights_initialize();

	return &export;
}
