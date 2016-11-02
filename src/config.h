#pragma once
#ifndef __SMARTHOME_CONFIG_H
#define __SMARTHOME_CONFIG_H

#include "defines.h"
#include "module.h"

// --------------------------------------------------------------------------------

void config_initialize(void);
void config_shutdown(void);

void config_add_command_handler(const char *command, config_handler_t method);
void config_parse_file(const char *path);
void config_parse_line(char *text);

#endif
