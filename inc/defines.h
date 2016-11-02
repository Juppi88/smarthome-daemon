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
		list = entry;\
	}

#define LIST_REMOVE_ENTRY(type, entry, list)\
	if (entry != list) {\
		for (type *tmp = list, *prev_tmp = NULL; tmp != NULL; prev_tmp = tmp, tmp = tmp->next) {\
			if (tmp == entry) {\
				prev_tmp->next = tmp->next;\
				break;\
			}\
		}\
	}\
	else {\
		list = entry->next;\
	}

#define LIST_FOREACH(type, var, list)\
	for (type *var = list; var != NULL; var = var->next)

#define LIST_FOREACH_SAFE(type, var, tmp, list)\
	for (type *var = list, *tmp = NULL; var != NULL; var = tmp)

#endif
