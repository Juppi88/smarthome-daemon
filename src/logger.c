#include "logger.h"
#include <stdio.h>
#include <stdarg.h>

void output_log(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	fprintf(stdout, "\n");
}

void output_error(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}
