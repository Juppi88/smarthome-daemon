#pragma once
#ifndef __SMARTHOME_LIGHTS_API_H
#define __SMARTHOME_LIGHTS_API_H

#include "module.h"

struct lights_api_t {
	// Set an override minimum brightness for a light, as a percentage (0...1) of the light's max brightness.
	// A value different than 0 will toggle the light on if necessary, 0 will toggle the light off if it is not toggled by the user.
	void (*set_min_brightness)(const char *light_id, float percentage);
};

#endif
