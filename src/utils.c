#include "utils.h"
#include "logger.h"
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

void *utils_alloc(size_t size)
{
	void *ptr = malloc(size);

	if (ptr == NULL) {
		output_error("Unable to allocate memory");
		exit(0);
	}

	memset(ptr, 0, size);
	return ptr;
}

void utils_free(void *ptr)
{
	free(ptr);
}

char *utils_duplicate_string(const char *text)
{
	if (text == NULL) {
		return NULL;
	}

	char *buf = utils_alloc(strlen(text) + 1), *s = buf;
	
	while (*text) {
		*s++ = *text++;
	}

	*s = 0;
	return buf;
}

char *utils_tokenize_string(const char *text, char delimiter, char *dst, size_t dst_len)
{
	static const char *s = NULL;

	if (text != NULL) {
		s = text;
	}

	// Skip leading delimiter characters.
	while (*s == delimiter) { ++s; }

	char *d = dst;

	// Copy into the destination buffer until the string end or delimiter is met.
	while (*s && dst_len-- > 0) {
		if (*s == delimiter) {
			break;
		}

		*d++ = *s++;
	}

	// Null terminate the destination buffer and return it.
	*d = 0;
	return dst;
}

#ifdef _WIN32

#include <Windows.h>
#include <process.h>

void utils_thread_create(thread_t func, void *args)
{
	uint32_t thread_addr;
	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, func, args, 0, &thread_addr);

	if (thread != NULL) {
		CloseHandle(thread);
	}
}

void utils_thread_sleep(uint32_t ms)
{
	Sleep(ms);
}

void *utils_load_library(const char *path)
{
	void *handle = (void *)GetModuleHandleA(path);

	if (handle == NULL) {
		handle = (void *)LoadLibraryA(path);
	}

	return handle;
}

void utils_close_library(void *handle)
{
	if (handle != NULL) {
		FreeLibrary((HMODULE)handle);
	}
}

void *utils_load_library_symbol(void *handle, const char *name)
{
	if (handle != NULL) {
		return (void *)GetProcAddress((HMODULE)handle, name);
	}

	return NULL;
}

#else

#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#include <unistd.h>

void utils_thread_create(thread_t func, void *args)
{
	pthread_t thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread, &attr, func, args);
}

void utils_thread_sleep(uint32_t ms)
{
	usleep(1000 * ms);
}

void *utils_load_library(const char *path)
{
	return dlopen(path, RTLD_NOW);
}

void utils_close_library(void *handle)
{
	if (handle != NULL) {
		dlclose(handle);
	}
}

void *utils_load_library_symbol(void *handle, const char *name)
{
	if (handle != NULL) {
		return dlsym(handle, name);
	}

	return NULL;
}

#endif
