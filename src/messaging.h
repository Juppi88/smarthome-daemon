#pragma once
#ifndef __SMARTHOME_MESSAGING_H
#define __SMARTHOME_MESSAGING_H

#include "defines.h"
#include "module.h"

void messaging_initialize(void);
void messaging_shutdown(void);
void messaging_publish(const char *message, const char *topic_fmt, ...);
void messaging_publish_data(void *data, size_t data_size, const char *topic_fmt, ...);
void messaging_subscribe(void *context, message_update_t callback, const char *topic_fmt, ...);
void messaging_unsubscribe(void *context, message_update_t callback, const char *topic_fmt, ...);

#endif
