#include "modules.h"
#include "module.h"
#include "utils.h"
#include "log.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

// --------------------------------------------------------------------------------

struct module_t {
	char *name;
	void *handle;
	struct module_export_t exports;
	struct module_t *next;
};

static struct module_import_t api;
static struct module_t *first;

#ifdef _WIN32
#define MODULE_EXTENSION ".dll"
#else
#define MODULE_EXTENSION ".so"
#endif

// --------------------------------------------------------------------------------

static void modules_load(const char *name);
static void modules_unload(struct module_t *module);
static struct module_t *modules_find(const char *name);
CONFIG_HANDLER(load_module);
CONFIG_HANDLER(unload_module);

void modules_initialize(void)
{
	api.log_write = output_log;
	api.log_write_error = output_error;

	// Register config handlers for module loading and unloading.
	config_add_command_handler("load_module", load_module);
	config_add_command_handler("unload_module", unload_module);
}

void modules_shutdown(void)
{
	for (struct module_t *mod = first, *tmp; mod != NULL; mod = tmp) {
		tmp = mod->next;
		modules_unload(mod);
	}

	first = NULL;
}

void modules_process(void)
{
	for (struct module_t *mod = first; mod != NULL; mod = mod->next) {
		if (mod->exports.process != NULL) {
			mod->exports.process();
		}
	}
}

static void modules_load(const char *name)
{
	// Make sure the module isn't already loaded.
	if (modules_find(name) != NULL) {
		output_log("Module '%s' is already loaded!", name);
		return;
	}

	char path[256];
	sprintf(path, "./modules/%s" MODULE_EXTENSION, name);

	// Try to open the library.
	void *handle = utils_load_library(path);

	if (handle == NULL) {
		output_log("Could not load module '%s': unable to load library", name);
		return;
	}

	// Try to resolve the initialization method.
	module_initialize_t init = (module_initialize_t)utils_load_library_symbol(handle, "module_initialize");

	if (init == NULL) {
		output_log("Could not load module '%s': unable to resolve initialization method", name);
		utils_close_library(handle);
		return;
	}

	// Call the initialization method and get module exports.
	struct module_export_t *modexport = init(&api);

	if (modexport == NULL || modexport->api_version != MODULE_API_VERSION) {
		output_log("Could not load module '%s': no exports or export API version not supported", name);
		utils_close_library(handle);
		return;
	}

	// Everything is fine, add the module to the loaded module list.
	struct module_t *module = utils_alloc(sizeof(*module));
	module->name = utils_duplicate_string(name);
	module->handle = handle;
	module->exports = *modexport;

	module->next = first;
	first = module;

	output_log("Loaded module '%s'", module->name);
}

static void modules_unload(struct module_t *module)
{
	if (first == module) {
		first = module->next;
	}

	// Call the shutdown method for the module.
	if (module->exports.shutdown != NULL) {
		module->exports.shutdown();
	}

	// Close the library handle.
	utils_close_library(module->handle);

	output_log("Unloaded module '%s'", module->name);

	utils_free(module->name);
	utils_free(module);
}

static struct module_t *modules_find(const char *name)
{
	for (struct module_t *mod = first; mod != NULL; mod = mod->next) {
		if (strcmp(mod->name, name) == 0) {
			return mod;
		}
	}

	return NULL;
}

CONFIG_HANDLER(load_module)
{
	if (*args == 0) {
		output_log("Usage: load_module <name>");
		return;
	}

	modules_load(args);
}

CONFIG_HANDLER(unload_module)
{
	if (*args == 0) {
		output_log("Usage: unload_module <name>");
		return;
	}

	struct module_t *mod = modules_find(args);

	if (mod != NULL) {
		modules_unload(mod);
	}
	else {
		output_log("Module '%s' is not loaded!", args);
	}
}
