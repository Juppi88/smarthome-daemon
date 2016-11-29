#include "main.h"
#include "modules.h"
#include "module.h"
#include "utils.h"
#include "logger.h"
#include "config.h"
#include "messaging.h"
#include "webapi.h"
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
static struct module_t *modules;

#ifdef _WIN32
#define MODULE_EXTENSION ".dll"
#else
#define MODULE_EXTENSION ".so"
#endif

// --------------------------------------------------------------------------------

static void modules_load(const char *name);
static void modules_unload(struct module_t *module);
static struct module_t *modules_find(const char *name);
static void *modules_get_api_pointer(const char *name);

CONFIG_HANDLER(load_module);
CONFIG_HANDLER(unload_module);

// --------------------------------------------------------------------------------

void modules_initialize(void)
{
	// Initialize the module API.
	api.get_config_directory = get_config_directory;
	api.config_add_command_handler = config_add_command_handler;
	api.config_parse_file = config_parse_file;
	api.log_write = output_log;
	api.log_write_error = output_error;
	api.message_publish = messaging_publish;
	api.message_publish_data = messaging_publish_data;
	api.message_subscribe = messaging_subscribe;
	api.message_unsubscribe = messaging_unsubscribe;
	api.webapi_register_interface = webapi_register_interface;
	api.webapi_unregister_interface = webapi_unregister_interface;
	api.get_module_api = modules_get_api_pointer;
	api.alloc = utils_alloc;
	api.free = utils_free;
	api.duplicate_string = utils_duplicate_string;
	api.tokenize_string = utils_tokenize_string;

	// Register config handlers for module loading and unloading.
	config_add_command_handler("load_module", load_module);
	config_add_command_handler("unload_module", unload_module);
}

void modules_shutdown(void)
{
	LIST_FOREACH_SAFE(struct module_t, mod, tmp, modules) {
		tmp = mod->next;
		modules_unload(mod);
	}

	modules = NULL;
}

void modules_process(void)
{
	LIST_FOREACH(struct module_t, mod, modules) {
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
	snprintf(path, sizeof(path), "./modules/%s" MODULE_EXTENSION, name);

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

	// Everything is fine, populate the module info.
	struct module_t *module = utils_alloc(sizeof(*module));
	module->name = utils_duplicate_string(name);
	module->handle = handle;
	module->exports = *modexport;

	// Inform the loaded module about other modules by calling the on_module_load method.
	if (module->exports.on_module_loaded != NULL) {
		LIST_FOREACH(struct module_t, mod, modules) {
			module->exports.on_module_loaded(mod->name);
		}
	}

	// Inform all loaded modules about the new module.
	LIST_FOREACH(struct module_t, mod, modules) {
		if (mod->exports.on_module_loaded != NULL) {
			mod->exports.on_module_loaded(module->name);
		}
	}

	// Add the module to the loaded module list.
	LIST_ADD_ENTRY(modules, module);

	output_log("Loaded module '%s'", module->name);
}

static void modules_unload(struct module_t *module)
{
	// Remove the module from the loaded mod list.
	LIST_REMOVE_ENTRY(struct module_t, module, modules);

	// Inform all the other loaded modules about the unloaded module.
	LIST_FOREACH(struct module_t, mod, modules) {
		if (mod->exports.on_module_unloaded != NULL) {
			mod->exports.on_module_unloaded(module->name);
		}
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
	LIST_FOREACH(struct module_t, mod, modules) {
		if (strcmp(mod->name, name) == 0) {
			return mod;
		}
	}

	return NULL;
}

static void *modules_get_api_pointer(const char *name)
{
	if (name == NULL) {
		return NULL;
	}

	struct module_t *module = modules_find(name);
	if (module == NULL) {
		return NULL;
	}

	return module->exports.api;
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
