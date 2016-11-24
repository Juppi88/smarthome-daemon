#pragma once
#ifndef __SMARTHOME_ALARM_MAIN_H
#define __SMARTHOME_ALARM_MAIN_H

#include "defines.h"
#include "module.h"
#include "../lights/lights_api.h"

extern struct module_import_t api;
extern struct module_export_t export;
extern struct lights_api_t *mod_lights;

#endif
