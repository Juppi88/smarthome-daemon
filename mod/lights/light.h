#pragma once
#ifndef __SMARTHOME_LIGHTS_LIGHT_H
#define __SMARTHOME_LIGHTS_LIGHT_H

#include "main.h"

#define DEFAULT_PWM_SIZE 10
#define DEFAULT_TRANSITION_TIME 1000
#define MAX_BRIGHTNESS(pwm) ((1 << pwm) - 1)

struct light_t {
	char *identifier;
	char *name;
	bool has_subscribed;
	bool is_enabled;
	bool is_toggled;
	uint8_t pwm_size;
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
