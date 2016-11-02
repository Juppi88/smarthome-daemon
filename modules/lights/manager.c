#include "manager.h"
#include <stdio.h>

#ifdef _WIN32
#include "../src/dirent.h"
#else
#include <dirent.h>
#endif

// --------------------------------------------------------------------------------

static struct light_t *lights;

// --------------------------------------------------------------------------------

void lights_initialize(void)
{
	char config_directory[260];
	snprintf(config_directory, sizeof(config_directory), "%s/lights", api.get_config_directory());
	
	// Load all the config files from the light config folder.
	DIR *dir = opendir(config_directory);

	if (dir == NULL) {
		return;
	}

	struct dirent *ent;

	while ((ent = readdir(dir)) != NULL)
	{
		char *dot = strrchr(ent->d_name, '.');
		
		if (ent->d_type == DT_REG && // Make sure the entry is a regular file...
			dot != NULL && strcmp(dot, ".conf") == 0) // ...and it is a config file (or at least pretends to be).
		{
			char file[512];
			sprintf(file, "%s/%s", config_directory, ent->d_name);
			
			struct light_t *light = light_create(file);

			// If the light was loaded from the file successfully, add it to a list of lights.
			LIST_ADD_ENTRY(lights, light);
		}
	}

	closedir(dir);
}

void lights_shutdown(void)
{
	// Destroy all loaded lights.
	LIST_FOREACH_SAFE(struct light_t, light, tmp, lights) {
		tmp = light->next;
		light_destroy(light);
	}
}

void lights_process(void)
{
	LIST_FOREACH(struct light_t, light, lights) {
		// TODO: Actually process, or something.
	}
}
