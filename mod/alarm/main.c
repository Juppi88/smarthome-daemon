#include "main.h"
#include "alarm.h"

struct module_import_t api;
struct module_export_t export;

// --------------------------------------------------------------------------------

MODULE_API struct module_export_t *module_initialize(struct module_import_t *import)
{
	// Store the smarthome API object.
	api = *import;

	// Initialize the export object and return it.
	export.api_version = MODULE_API_VERSION;
	export.process = alarm_process;
	export.shutdown = alarm_shutdown;

	// Setup this module.
	alarm_initialize();

	return &export;
}
