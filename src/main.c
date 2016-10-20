#include "config.h"
#include "modules.h"
#include "messaging.h"
#include "utils.h"
#include <stdio.h>

static bool running = true;

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
		fgets(input, sizeof(input), stdin);
	}

	return 0;
}

#endif

int main(int argc, char **argv)
{
	// Initialize subsystems.
	config_initialize();
	messaging_initialize();
	modules_initialize();

	// Load the config file. The main config file will contain the modules to be loaded.
	config_parse_file("./config/smarthome.conf");

#ifndef DAEMON
	// Register a command for shutting down the process.
	config_add_command_handler("quit", quit);

	// Create a thread to read console input.
	utils_thread_create(input_thread, NULL);
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
		utils_thread_sleep(10);
	}

	// Unload modules and shutdown subsystems.
	modules_shutdown();
	messaging_shutdown();
	config_shutdown();

	return 0;
}
