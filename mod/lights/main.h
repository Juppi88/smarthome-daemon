#pragma once
#ifndef __SMARTHOME_LIGHTS_MAIN_H
#define __SMARTHOME_LIGHTS_MAIN_H

#include "defines.h"
#include "module.h"
#include "../alarm/alarm_api.h"

extern struct module_import_t api;
extern struct module_export_t export;
extern struct alarm_api_t *mod_alarm;

#endif
