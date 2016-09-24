#pragma once
#ifndef __SMARTHOME_UTILS_H
#define __SMARTHOME_UTILS_H

#include "defines.h"

#ifdef _WIN32
	typedef uint32_t (__stdcall *thread_t)(void *args);
	#define THREAD(x) uint32_t __stdcall x(void *args)
#else
	typedef void *(*thread_t)(void *args);
	#define THREAD(x) void *x(void *args)
#endif

void *utils_alloc(size_t size);
void utils_free(void *ptr);

char *utils_duplicate_string(const char *text);

void utils_thread_create(thread_t method, void *args);
void utils_thread_sleep(uint32_t ms);

void *utils_load_library(const char *path);
void utils_close_library(void *handle);
void *utils_load_library_symbol(void *handle, const char *name);

#endif
