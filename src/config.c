#include "config.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>

// --------------------------------------------------------------------------------

struct handler_t {
	char *command;
	config_handler_t method;
	struct handler_t *next;
};

static struct handler_t *first;

// --------------------------------------------------------------------------------

void config_initialize(void)
{
}

void config_shutdown(void)
{
	for (struct handler_t *p = first, *tmp; p != NULL; p = tmp) {
		tmp = p->next;
		utils_free(p->command);
		utils_free(p);
	}

	first = NULL;
}

static struct handler_t *config_find_command_handler(const char *command)
{
	for (struct handler_t *handler = first;handler != NULL; handler = handler->next) {
		if (strcmp(command, handler->command) == 0) {
			return handler;
		}
	}

	return NULL;
}

void config_add_command_handler(const char *command, config_handler_t method)
{
	if (command == NULL || method == NULL) {
		return;
	}

	struct handler_t *handler = config_find_command_handler(command);

	// The handler already exists, update the method.
	if (handler != NULL) {
		handler->method = method;
		return;
	}

	// Handler doesn't exist yet, create it and add it to the list.
	handler = utils_alloc(sizeof(*handler));
	handler->command = utils_duplicate_string(command);
	handler->method = method;

	handler->next = first;
	first = handler;
}

void config_parse_file(const char *path)
{
	FILE *f = fopen(path, "r");

	if (f == NULL) {
		return;
	}

	char line[256], c = 0;
	size_t i = 0;

	while (c != EOF) {
		// Read the file line by line.
		do {
			c = getc(f);

			if (c == EOF || c == '\n' || c == '\r') {
				break;
			}

			line[i++] = c;
		} while (c != '\0' && c != -1 && i < sizeof(line) - 1);

		// Null-terminate and parse all non-empty lines.
		if (i != 0) {
			line[i] = 0;
			config_parse_line(line);

			i = 0;
		}
	}

	fclose(f);
}

void config_parse_line(char *text)
{
	register char *s = text;
	char *cmd, *args;
	struct handler_t *handler;
	
	// Strip leading whitespace.
	while (*s == ' ' || *s == '\t') {
		++s;
	}

	// Ignore comment lines.
	if (*s == '#' || *s == 0) {
		return;
	}

	// Get the command text and null-terminate it.
	cmd = s;

	for (;;) {
		switch (*s) {
		case 0:
		case '\r':
		case '\n':
		case '\t':
		case ' ':
			goto parse_args;

		default:
			++s;
			break;
		}
	}

parse_args:

	if (*s == 0) {
		args = s;
		goto handle_command;
	}

	*s++ = 0;
	
	// Strip leading whitespace.
	while (*s == ' ' || *s == '\t') {
		++s;
	}

	// Get the arguments text and null-terminate it.
	args = s;

	for (;;) {
		switch (*s) {
		case 0:
		case '\r':
		case '\n':
			*s = 0;
			goto handle_command;

		default:
			++s;
			break;
		}
	}
	
handle_command:

	// Call the handler method for this config command if it exists.
	handler = config_find_command_handler(cmd);
	
	if (handler != NULL) {
		handler->method(args);
	}
}
