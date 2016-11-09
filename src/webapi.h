#pragma once
#ifndef __SMARTHOME_WEBAPI_H
#define __SMARTHOME_WEBAPI_H

#include "defines.h"
#include "module.h"

void webapi_initialize(void);
void webapi_shutdown(void);
void webapi_process(void);
void webapi_register_interface(const char *iface, web_api_handler_t handler);
void webapi_unregister_interface(const char *iface);

#endif
