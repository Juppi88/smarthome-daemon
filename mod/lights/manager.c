#include "manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include "../src/dirent.h"
#else
#include <dirent.h>
#ifndef DT_DIR
#define DT_DIR 0x4
#endif
#endif

// --------------------------------------------------------------------------------

static struct light_t *lights;

// --------------------------------------------------------------------------------

static struct light_t *lights_get_light(const char *identifier);

WEB_API_HANDLER(lights_process_api_request);

// --------------------------------------------------------------------------------

void lights_initialize(void)
{
	char config_directory[260];
	snprintf(config_directory, sizeof(config_directory), "%s/lights", api.get_config_directory());

	// Load all the config files from the light config folder.
	DIR *dir = opendir(config_directory);

	if (dir == NULL) {
		return;
	}

	struct dirent *ent;

	while ((ent = readdir(dir)) != NULL)
	{
		char *dot = strrchr(ent->d_name, '.');
		if (ent->d_type != DT_DIR && // Make sure the entry is a regular file...
			dot != NULL && strcmp(dot, ".conf") == 0) // ...and it is a config file (or at least pretends to be).
		{
			char file[512];
			sprintf(file, "%s/%s", config_directory, ent->d_name);

			struct light_t *light = light_create(file);

			// If the light was loaded from the file successfully, add it to a list of lights.
			LIST_ADD_ENTRY(lights, light);
		}
	}

	// Register a handler for web API requests.
	api.webapi_register_interface("lights", lights_process_api_request);

	closedir(dir);
}

void lights_shutdown(void)
{
	// Destroy all loaded lights.
	LIST_FOREACH_SAFE(struct light_t, light, tmp, lights) {
		tmp = light->next;
		light_destroy(light);
	}

	api.webapi_unregister_interface("lights");
}

void lights_process(void)
{
	LIST_FOREACH(struct light_t, light, lights) {
		// TODO: Actually process, or something.
	}
}

static struct light_t *lights_get_light(const char *identifier)
{
	LIST_FOREACH(struct light_t, light, lights) {
		if (strcmp(light->identifier, identifier) == 0) {
			return light;
		}
	}

	return NULL;
}

#define ADVANCE_BUFFER(buffer, bufvar, written)\
	bufvar = buffer + written;\
	if (written >= sizeof(buffer))\
		return true;

WEB_API_HANDLER(lights_process_api_request)
{
	char interface[128], command[128], light_name[128], value[128];

	// Get the command token from the request URL.
	api.tokenize_string(request_url, '/', interface, sizeof(interface));
	api.tokenize_string(NULL, '/', command, sizeof(command));

	// Get the list of all lights and their statuses.
	if (strcmp(command, "status") == 0) {

		static char buffer[10000];

		char *s = buffer;
		size_t written = 0;
		const size_t size = sizeof(buffer);

		*content = buffer;

		// Awful shit, this right here is. Yoda I am.
		written += snprintf(s, size - written, "{\n\"lights\":[\n"); ADVANCE_BUFFER(buffer, s, written);

		LIST_FOREACH(struct light_t, light, lights) {
			written += snprintf(s, size - written, "\t{\n"); ADVANCE_BUFFER(buffer, s, written);
			written += snprintf(s, size - written, "\t\t\"identifier\": \"%s\",\n", light->identifier); ADVANCE_BUFFER(buffer, s, written);
			written += snprintf(s, size - written, "\t\t\"name\": \"%s\",\n", light->name); ADVANCE_BUFFER(buffer, s, written);
			written += snprintf(s, size - written, "\t\t\"toggled\": \"%u\",\n", light->is_toggled ? 1 : 0); ADVANCE_BUFFER(buffer, s, written);
			written += snprintf(s, size - written, "\t\t\"max_brightness\": \"%u\"\n", light->max_brightness); ADVANCE_BUFFER(buffer, s, written);
			written += snprintf(s, size - written, "\t}%s\n", light->next != NULL ? "," : ""); ADVANCE_BUFFER(buffer, s, written);
		}

		written += snprintf(s, size - written, "]\n}\n"); ADVANCE_BUFFER(buffer, s, written);

		return true;
	}

	// Toggle the light on and off.
	else if (strcmp(command, "toggle") == 0) {

		api.tokenize_string(NULL, '/', light_name, sizeof(light_name));
		api.tokenize_string(NULL, '/', value, sizeof(value));

		struct light_t *light = lights_get_light(light_name);

		if (light != NULL && value[0] != 0) {
			bool toggle = (value[0] != '0');
			light_set_toggled(light, toggle);

			return true;
		}
	}

	// Set the max brightness for the light.
	else if (strcmp(command, "max_brightness") == 0) {

		api.tokenize_string(NULL, '/', light_name, sizeof(light_name));
		api.tokenize_string(NULL, '/', value, sizeof(value));

		struct light_t *light = lights_get_light(light_name);

		if (light != NULL && value[0] != 0) {

			uint16_t brightness = (uint16_t)atoi(value);
			light_set_max_brightness(light, brightness);

			return true;
		}
	}

	// Request was not recognised.
	return false;
}
