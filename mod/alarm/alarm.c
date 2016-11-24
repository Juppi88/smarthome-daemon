#include "alarm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// --------------------------------------------------------------------------------

enum {
	DAY_MON = 0x01,
	DAY_TUE = 0x02,
	DAY_WED = 0x04,
	DAY_THU = 0x08,
	DAY_FRI = 0x10,
	DAY_SAT = 0x20,
	DAY_SUN = 0x40,
	DAYS_ALL = (DAY_MON | DAY_TUE | DAY_WED | DAY_THU | DAY_FRI | DAY_SAT | DAY_SUN)
};

struct alarm_t {
	uint8_t identifier;
	uint8_t hour;
	uint8_t minute;
	uint8_t days;
	struct alarm_t *next;
};

struct alarm_light_t {
	char *identifier;
	struct alarm_light_t *next;
};

#define MAX_WAKEUP_TIME 15
#define MAX_KEEPON_TIME 60

static uint8_t alarm_wakeup_time = 10; // Minutes before the alarm to start turning the lights on
static uint8_t alarm_keepon_time = 45; // Minutes after the alarm to keep the lights toggled on

static uint32_t alarm_count = 0; // Number of active alarms
static struct alarm_t *alarms; // List of alarm entries
static struct alarm_light_t *alarm_lights; // List of lights attached to the alarm system

// --------------------------------------------------------------------------------

static struct alarm_t *alarm_add(void);
static struct alarm_t *alarm_find(uint8_t identifier);
static void alarm_remove(struct alarm_t *alarm);
static void alarm_sort_all(void);

CONFIG_HANDLER(alarm_add_light);
WEB_API_HANDLER(alarm_process_api_request);

// --------------------------------------------------------------------------------

void alarm_initialize(void)
{
	// Set some config command handlers.
	api.config_add_command_handler("alarm_light", alarm_add_light);

	// Load the config file.
	char config_file[260];
	snprintf(config_file, sizeof(config_file), "%s/alarm/alarm.conf", api.get_config_directory());

	api.config_parse_file(config_file);

	// Register a handler for web API requests.
	api.webapi_register_interface("alarm", alarm_process_api_request);
}

void alarm_shutdown(void)
{
	// Unregister the web API handler for this module.
	api.webapi_unregister_interface("alarm");

	// Destroy the light list used for alarms.
	LIST_FOREACH_SAFE(struct alarm_light_t, light, tmp, alarm_lights) {
		tmp = light->next;

		api.free(light->identifier);
		api.free(light);
	}

	alarm_lights = NULL;

	// Destroy all alarm entries.
	LIST_FOREACH_SAFE(struct alarm_t, alarm, tmp, alarms) {
		tmp = alarm->next;
		api.free(alarm);
	}

	alarms = NULL;
	alarm_count = 0;
}

void alarm_process(void)
{
	LIST_FOREACH(struct alarm_t, alarm, alarms) {
		// PROCESS ME!
	}
}

static struct alarm_t *alarm_add(void)
{
	// The system only supports up to 10 simultaneous alarms for simplicity reasons.
	if (alarm_count >= MAX_ALARMS) {
		return NULL;
	}

	struct alarm_t *alarm = api.alloc(sizeof(*alarm));

	// Default the wakeup time to the current time.
	time_t rawtime;
	time(&rawtime);

	struct tm *info = localtime(&rawtime);
	alarm->hour = (uint8_t)info->tm_hour;
	alarm->minute = (uint8_t)info->tm_min;

	// Add the alarm to the list and sort it (give every alarm entry a unique identifier).
	LIST_ADD_ENTRY(alarms, alarm);
	++alarm_count;

	alarm_sort_all();

	return alarm;
}

static struct alarm_t *alarm_find(uint8_t identifier)
{
	LIST_FOREACH(struct alarm_t, alarm, alarms) {
		if (alarm->identifier == identifier) {
			return alarm;
		}
	}

	return NULL;
}

static void alarm_remove(struct alarm_t *alarm)
{
	LIST_REMOVE_ENTRY(struct alarm_t, alarm, alarms);
	--alarm_count;

	alarm_sort_all();
}

static void alarm_sort_all(void)
{
	uint8_t identifier = 0;

	LIST_FOREACH(struct alarm_t, alarm, alarms) {
		alarm->identifier = ++identifier;
	}
}

CONFIG_HANDLER(alarm_add_light)
{
	// Make sure the light hasn't been added to the alarm system yet.
	LIST_FOREACH(struct alarm_light_t, light, alarm_lights) {
		if (strcmp(light->identifier, args) == 0) {
			return;
		}
	}

	// Add a light to the alarm. This light will be toggled on when the alarm is triggered.
	struct alarm_light_t *light = api.alloc(sizeof(*light));
	light->identifier = api.duplicate_string(args);

	LIST_ADD_ENTRY(alarm_lights, light);
}

#define ADVANCE_BUFFER(buffer, bufvar, written)\
	bufvar = buffer + written;\
	if (written >= sizeof(buffer))\
		return true;

WEB_API_HANDLER(alarm_process_api_request)
{
	char interface[128], command[128], alarm_id[128], args[128];

	// Get the command token from the request URL.
	api.tokenize_string(request_url, '/', interface, sizeof(interface));
	api.tokenize_string(NULL, '/', command, sizeof(command));

	// Return the status of the alarm system with a list of active alarms.
	if (strcmp(command, "status") == 0) {

		static char buffer[10000];

		char *s = buffer;
		size_t written = 0;
		const size_t size = sizeof(buffer);

		*content = buffer;

		written += snprintf(s, size - written, "{\n\"wakeup_time\": %u,\n", alarm_wakeup_time); ADVANCE_BUFFER(buffer, s, written);
		written += snprintf(s, size - written, "\"keepon_time\": %u,\n", alarm_keepon_time); ADVANCE_BUFFER(buffer, s, written);
		written += snprintf(s, size - written, "\"alarms\":[\n"); ADVANCE_BUFFER(buffer, s, written);
		
		LIST_FOREACH(struct alarm_t, alarm, alarms) {

			written += snprintf(s, size - written, "\t{\n"); ADVANCE_BUFFER(buffer, s, written);
			written += snprintf(s, size - written, "\t\t\"identifier\": \"%u\",\n", alarm->identifier); ADVANCE_BUFFER(buffer, s, written);
			written += snprintf(s, size - written, "\t\t\"hour\": \"%u\",\n", alarm->hour); ADVANCE_BUFFER(buffer, s, written);
			written += snprintf(s, size - written, "\t\t\"minute\": \"%u\",\n", alarm->minute); ADVANCE_BUFFER(buffer, s, written);
			written += snprintf(s, size - written, "\t\t\"days\": \"%u\"\n", alarm->days); ADVANCE_BUFFER(buffer, s, written);
			written += snprintf(s, size - written, "\t}%s\n", alarm->next != NULL ? "," : ""); ADVANCE_BUFFER(buffer, s, written);
		}
		
		written += snprintf(s, size - written, "]\n}\n"); ADVANCE_BUFFER(buffer, s, written);

		return true;
	}

	// Add a new alarm entry.
	else if (strcmp(command, "add") == 0) {

		// Add a new alarm. It's as simple as that!
		alarm_add();
		return true;
	}

	// Remove an existing alarm entry.
	else if (strcmp(command, "remove") == 0) {

		api.tokenize_string(NULL, '/', alarm_id, sizeof(alarm_id));

		if (*alarm_id) {
			struct alarm_t *alarm = alarm_find((uint8_t)atoi(alarm_id));
			if (alarm != NULL) {
				alarm_remove(alarm);
			}
		}

		return true;
	}

	// Modify an existing alarm entry.
	else if (strcmp(command, "edit") == 0) {

		api.tokenize_string(NULL, '/', alarm_id, sizeof(alarm_id));
		api.tokenize_string(NULL, '/', args, sizeof(args));

		if (*alarm_id && *args) {
			struct alarm_t *alarm = alarm_find((uint8_t)atoi(alarm_id));

			if (alarm != NULL) {

				// The argument string is formatted DAYS-HOUR-MIN, parse it accordingly.
				char days[12], hour[12], minute[12];
				api.tokenize_string(args, '-', days, sizeof(days));
				api.tokenize_string(NULL, '-', hour, sizeof(hour));
				api.tokenize_string(NULL, '-', minute, sizeof(minute));

				if (*days && *hour && *minute) {
					uint8_t alarm_days = (uint8_t)atoi(days);
					uint8_t alarm_hour = (uint8_t)atoi(hour);
					uint8_t alarm_minute = (uint8_t)atoi(minute);

					// Update the entry.
					alarm->days = (alarm_days & DAYS_ALL);
					alarm->hour = (alarm_hour < 24 ? alarm_hour : 0);
					alarm->minute = (alarm_minute < 60 ? alarm_minute : 0);
				}
			}
		}

		return true;
	}

	// Alter the wakeup period.
	else if (strcmp(command, "wakeup_time") == 0) {

		api.tokenize_string(NULL, '/', args, sizeof(args));

		if (*args) {
			uint32_t wakeup = (uint32_t)atoi(args);
			alarm_wakeup_time = (wakeup <= MAX_WAKEUP_TIME ? wakeup : MAX_WAKEUP_TIME);
		}

		return true;
	}

	// Alter the time to keep the light on after an alarm.
	else if (strcmp(command, "keepon_time") == 0) {

		api.tokenize_string(NULL, '/', args, sizeof(args));

		if (*args) {
			uint32_t keepon = (uint32_t)atoi(args);
			alarm_keepon_time = (keepon <= MAX_KEEPON_TIME ? keepon : MAX_KEEPON_TIME);
		}

		return true;
	}

	// Request was not recognised.
	return false;
}
