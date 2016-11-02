#pragma once
#ifndef __SMARTHOME_LIGHTS_MANAGER_H
#define __SMARTHOME_LIGHTS_MANAGER_H

#include "main.h"
#include "light.h"

void lights_initialize(void);
void lights_shutdown(void);
void lights_process(void);

#endif
