#pragma once
#ifndef __SMARTHOME_ALARM_API_H
#define __SMARTHOME_ALARM_API_H

#include "module.h"

struct alarm_api_t {
	// Notify the alarm system of changes in the light's state made by the user.
	// This will halt all on-going alarms.
	void (*on_light_state_changed)(const char *light_id);
};

#endif
