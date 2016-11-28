#include "alarm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// --------------------------------------------------------------------------------

enum {
	DAYS_NONE = 0,
	DAY_MON = (1 << 1),
	DAY_TUE = (1 << 2),
	DAY_WED = (1 << 3),
	DAY_THU = (1 << 4),
	DAY_FRI = (1 << 5),
	DAY_SAT = (1 << 6),
	DAY_SUN = (1 << 0),
	DAYS_ALL = (DAY_MON | DAY_TUE | DAY_WED | DAY_THU | DAY_FRI | DAY_SAT | DAY_SUN)
};

struct alarm_t {
	uint8_t identifier;
	uint8_t hour;		// The hour of the day for this alarm (0-23)
	uint8_t minute;		// The minute of the hour (0-59)
	uint8_t days;		// A bitfield representing the days this alarm is set to activate on (see the enum above)
	time_t alarm_time;	// The next time this alarm is set to trigger (UNIX timestamp)
	struct alarm_t *next;
};

struct alarm_light_t {
	char *identifier;
	struct alarm_light_t *next;
};

#define MAX_WAKEUP_TIME 15
#define MIN_KEEPON_TIME 1
#define MAX_KEEPON_TIME 60
#define ALARM_UPDATE_INTERVAL 10 // Process the alarm lights once every 10 seconds.

static uint8_t alarm_wakeup_time = 10; // Minutes before the alarm to start turning the lights on
static uint8_t alarm_keepon_time = 45; // Minutes after the alarm to keep the lights toggled on

static float alarm_light_brightness = 0.0f;	// Override brightness which is set when an alarm is in progress.

static uint32_t alarm_count = 0; // Number of active alarms
static struct alarm_t *alarms; // List of alarm entries
static struct alarm_light_t *alarm_lights; // List of lights attached to the alarm system

static time_t alarm_previous_update = 0;

// --------------------------------------------------------------------------------

static struct alarm_t *alarm_add(void);
static struct alarm_t *alarm_find(uint8_t identifier);
static void alarm_remove(struct alarm_t *alarm);
static void alarm_calculate_next_trigger_time(struct alarm_t *alarm);
static void alarm_sort_all(void);
static void alarm_set_override_light_brightness(float brightness);


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
	// Get the current time.
	time_t now = time(NULL);

	// We don't have to update the alarm system once every process cycle, a couple of times per minute is enough.
	if (now - alarm_previous_update < ALARM_UPDATE_INTERVAL) {
		return;
	}

	alarm_previous_update = now;

	// Find the highest progress for an alarm. This is used to set an override brightness for the lights.
	// If no alarms are in progress, the progress will be left to 0 and all lights will turn off.
	float alarm_progress = 0;

	LIST_FOREACH(struct alarm_t, alarm, alarms) {
		
		// Is the alarm in progress?
		if (alarm->alarm_time != 0 &&
			alarm->alarm_time - alarm_wakeup_time * 60 < now) {

			// If the alarm has ended, suspend it and calculate a new timestamp for the next alarm cycle.
			if (alarm->alarm_time + alarm_keepon_time * 60 < now) {
				alarm_calculate_next_trigger_time(alarm);
				continue;
			}

			// Calculate the progress for the alarm.
			float progress = (float)(now - alarm->alarm_time) / (alarm_wakeup_time * 60);
			progress = progress < 1 ? progress: 1;

			// If this is the active alarm which has progressed the furthest, use it to determine the brightness of the lights.
			if (progress > alarm_progress) {
				alarm_progress = progress;
			}
		}
	}

	// Set an override brightness for the alarm lights.
	alarm_set_override_light_brightness(alarm_progress);
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

static void alarm_calculate_next_trigger_time(struct alarm_t *alarm)
{
	// If the alarm is not set to activate on any day, it never triggers.
	if (alarm->days == DAYS_NONE) {
		alarm->alarm_time = 0;
		return;
	}

	// Get the current time info.
	time_t now = time(NULL);
	struct tm *info = localtime(&now);

	// If there is currently an alarm in progress, move the timestamp 'alarm_wakeup_time' minutes forward
	// to take into account the time it takes to bring on the lights. This will suspend ongoing alarms.
	if (alarm->alarm_time != 0 && alarm->alarm_time - alarm_wakeup_time * 60 < now) {

		info->tm_min += alarm_wakeup_time;
		now = mktime(info);

		info = localtime(&now);
	}
	
	// Find the day of the next alarm.
	int days_until_alarm = 0;
	int days_until_potential_alarm = -1;
	bool day_found = false;

	for (int day = info->tm_wday, c = 0; c < 7; ++c, ++day, ++days_until_alarm) {

		// Week days are numbered from 0...6 and they start from sunday.
		if (day > 6) {
			day = 0;
		}

		// There is an alarm set for this day. 
		if ((alarm->days & (1 << day)) != 0) {

			// If the time of the alarm hasn't passed yet, we've found our day. If not, continue the search.
			if (info->tm_hour > alarm->hour ||
				(info->tm_hour == alarm->hour && info->tm_min > alarm->minute)) {

				day_found = true;
				break;
			}

			// Mark the next potential alarm a week from the first potential match.
			if (days_until_potential_alarm < 0) {
				days_until_potential_alarm = days_until_alarm + 7;
			}
		}
	}

	// If we didn't find a potential candidate within the next 7 days, try the following week.
	if (!day_found) {
		days_until_alarm = days_until_potential_alarm;
		return;
	}

	// This should never happen! Let's make sure it doesn't anything in case it does, though.
	if (days_until_alarm < 0) {
		return;
	}

	// Advance the day counter to the day of the next alarm and set the time of the alarm, then get a timestamp for that.
	info->tm_mday += days_until_alarm;
	info->tm_hour = alarm->hour;
	info->tm_min = alarm->minute;
	info->tm_sec = 0;

	alarm->alarm_time = mktime(info);

	printf("(%d) Next alarm is set to %d.%d.%d at %02d:%02d\n", alarm->identifier, info->tm_mday, info->tm_mon, info->tm_year, info->tm_hour, info->tm_min);
}

static void alarm_sort_all(void)
{
	uint8_t identifier = 0;

	LIST_FOREACH(struct alarm_t, alarm, alarms) {
		alarm->identifier = ++identifier;
	}
}

static void alarm_set_override_light_brightness(float brightness)
{
	if (brightness == alarm_light_brightness) {
		return;
	}

	if (mod_lights != NULL) {
		LIST_FOREACH(struct alarm_light_t, light, alarm_lights) {
			mod_lights->set_min_brightness(light->identifier, brightness);
		}
	}

	alarm_light_brightness = brightness;
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
		time_t now = time(NULL) + alarm_wakeup_time * 60; // Alarm starts 'alarm_wakeup_time' minutes before the set time to bring on the lights gradually.

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
			written += snprintf(s, size - written, "\t\t\"days\": \"%u\",\n", alarm->days); ADVANCE_BUFFER(buffer, s, written);
			written += snprintf(s, size - written, "\t\t\"in_progress\": \"%s\"\n", (alarm->alarm_time != 0 && alarm->alarm_time <= now ? "true" : "false")); ADVANCE_BUFFER(buffer, s, written);
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

	// Suspend an alarm in progress.
	else if (strcmp(command, "suspend") == 0) {

		api.tokenize_string(NULL, '/', alarm_id, sizeof(alarm_id));

		if (*alarm_id) {

			struct alarm_t *alarm = alarm_find((uint8_t)atoi(alarm_id));

			// Recalculate the time for the next alarm. This will suspend the currently active alarm.
			if (alarm != NULL) {
				alarm_calculate_next_trigger_time(alarm);

				// Force a re-evaluation of the current alarm progress.
				alarm_previous_update = 0;
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

					// Recalculate the timestamp for the next alarm. This will suspend the alarm if it is currently in progress.
					alarm_calculate_next_trigger_time(alarm);
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
			alarm_keepon_time = (keepon >= MIN_KEEPON_TIME ? keepon : MIN_KEEPON_TIME);
			alarm_keepon_time = (alarm_keepon_time <= MAX_KEEPON_TIME ? alarm_keepon_time : MAX_KEEPON_TIME);
		}

		return true;
	}

	// Request was not recognised.
	return false;
}
