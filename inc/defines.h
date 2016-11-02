#pragma once
#ifndef __SMARTHOME_DEFINES_H
#define __SMARTHOME_DEFINES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef _WIN32
#define INLINE __inline
#else
#define INLINE inline
#endif

// Linked list macros

#define LIST_ADD_ENTRY(list, entry)\
	if (entry != NULL) {\
		if (list != NULL) {\
			entry->next = list;\
		}\
		entry = light;\
	}

#define LIST_FOREACH(type, var, list)\
	for (type *var = list; var != NULL; var = var->next)

#define LIST_FOREACH_SAFE(type, var, tmp, list)\
	for (type *var = list, *tmp = NULL; var != NULL; var = tmp)

#endif
