#pragma once
#ifndef __SMARTHOME_LIGHTS_LIGHT_H
#define __SMARTHOME_LIGHTS_LIGHT_H

#include "main.h"

#define MAX_BRIGHTNESS 255
#define TRANSITION_TIME 1000

struct light_t {
	char *identifier;
	char *name;
	bool has_subscribed;
	bool is_toggled;
	uint16_t max_brightness;
	uint16_t transition_time;
	struct light_t *next;
};

struct light_t *light_create(const char *config_file);
void light_destroy(struct light_t *light);

void light_set_toggled(struct light_t *light, bool toggle);
void light_set_max_brightness(struct light_t *light, uint16_t max_brightness);
void light_set_transition_time(struct light_t *light, uint16_t transition_time);

#endif
