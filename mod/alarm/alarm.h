#pragma once
#ifndef __SMARTHOME_ALARM_H
#define __SMARTHOME_ALARM_H

#include "main.h"

#define MAX_ALARMS 10

void alarm_initialize(void);
void alarm_shutdown(void);
void alarm_process(void);

#endif
