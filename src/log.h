#pragma once
#ifndef __SMARTHOME_LOG_H
#define __SMARTHOME_LOG_H

#include "defines.h"

void output_log(const char *format, ...);
void output_error(const char *format, ...);

#endif
