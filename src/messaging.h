#pragma once
#ifndef __SMARTHOME_MESSAGING_H
#define __SMARTHOME_MESSAGING_H

#include "defines.h"
#include "module.h"

void messaging_initialize(void);
void messaging_shutdown(void);
void messaging_publish(const char *topic, const char *message);
void messaging_subscribe(const char *topic, void *context, message_update_t callback);
void messaging_unsubscribe(const char *topic, void *context, message_update_t callback);

#endif
