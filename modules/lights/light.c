#include "light.h"
#include <stdio.h>
#include <stdlib.h>

static struct light_t *current_light;

#define LIGHT_TOGGLE_TOPIC "lights/%s/toggle"
#define LIGHT_MAX_BRIGHTNESS_TOPIC "lights/%s/max_brightness"

// --------------------------------------------------------------------------------

CONFIG_HANDLER(set_light_identifier);
CONFIG_HANDLER(set_light_name);
MESSAGE_HANDLER(update_light_toggle);
MESSAGE_HANDLER(update_light_max_brightness);

struct light_t *light_create(const char *config_file)
{
	current_light = api.alloc(sizeof(*current_light));

	// Set listeners for the config keys and parse the config file.
	api.config_add_command_handler("light_id", set_light_identifier);
	api.config_add_command_handler("light_name", set_light_name);
	
	api.config_parse_file(config_file);

	// Make sure the config file contained all the necessary info.
	if (current_light->identifier == NULL ||
		current_light->name == NULL) {

		light_destroy(current_light);
		return NULL;
	}

	current_light->max_brightness = MAX_BRIGHTNESS;

	// Register listeners for the messages regarding the status of the light.
	// We should be receiving the current states as soon as messaging is initialized.
	api.message_subscribe(current_light, update_light_toggle, LIGHT_TOGGLE_TOPIC, current_light->identifier);
	api.message_subscribe(current_light, update_light_max_brightness, LIGHT_MAX_BRIGHTNESS_TOPIC, current_light->identifier);

	current_light->has_subscribed = true;

	return current_light;
}

void light_destroy(struct light_t *light)
{
	// Unregister all message listeners.
	if (light->has_subscribed) {
		api.message_unsubscribe(light, update_light_toggle, LIGHT_TOGGLE_TOPIC, light->identifier);
		api.message_unsubscribe(light, update_light_max_brightness, LIGHT_MAX_BRIGHTNESS_TOPIC, light->identifier);
	}

	api.free(light->identifier);
	api.free(light->name);
	api.free(light);
}

CONFIG_HANDLER(set_light_identifier)
{
	if (current_light == NULL || args == NULL || *args == 0) {
		return;
	}

	if (current_light->identifier != NULL) {
		api.free(current_light->identifier);
	}

	current_light->identifier = api.duplicate_string(args);
}

CONFIG_HANDLER(set_light_name)
{
	if (current_light == NULL || args == NULL || *args == 0) {
		return;
	}

	if (current_light->name != NULL) {
		api.free(current_light->name);
	}

	current_light->name = api.duplicate_string(args);
}

MESSAGE_HANDLER(update_light_toggle)
{
	struct light_t *light = (struct light_t *)context;
	light->is_toggled = (atoi(message) != 0);
}

MESSAGE_HANDLER(update_light_max_brightness)
{
	struct light_t *light = (struct light_t *)context;
	light->max_brightness = (uint16_t)atoi(message);
}
