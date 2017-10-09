#include "main.h"
#include "config.h"
#include "modules.h"
#include "messaging.h"
#include "webapi.h"
#include "utils.h"
#include "logger.h"
#include <stdio.h>

static bool running = true;
static char config_directory[260] = { "./config" };

#ifndef DAEMON

static char input[512];

CONFIG_HANDLER(quit)
{
	running = false;
}

THREAD(input_thread)
{
	(void)args;

	while (running) {
		char * text = fgets(input, sizeof(input), stdin);
		(void)text;
	}

	return 0;
}

MESSAGE_HANDLER(debug)
{
	output_log("Debug message: %s", message);
}

#endif

int main(int argc, char **argv)
{
	// Initialize subsystems.
	config_initialize();
	messaging_initialize();
	webapi_initialize();
	modules_initialize();

	// Load the config file. The main config file will contain the modules to be loaded.
	char config_file[260];
	snprintf(config_file, sizeof(config_file), "%s/smarthome.conf", config_directory);

	config_parse_file(config_file);

#ifndef DAEMON
	// Register a command for shutting down the process.
	config_add_command_handler("quit", quit);

	// Create a thread to read console input.
	utils_thread_create(input_thread, NULL);

	// Register a listener for debug messages pushed over MQTT.
	messaging_subscribe(NULL, debug, "debug");
#endif

	while (running) {

#ifndef DAEMON
		// Process console input if this is running as a regular process.
		if (*input != 0) {
			config_parse_line(input);
			*input = 0;
		}
#endif

		// Process subsystems and modules.
		modules_process();
		webapi_process();

		// No need to sleep here because the web API blocks for 10ms.
	}

	// Unload modules and shutdown subsystems.
	modules_shutdown();
	webapi_shutdown();
	messaging_shutdown();
	config_shutdown();

	return 0;
}

const char *get_config_directory(void)
{
	return config_directory;
}
