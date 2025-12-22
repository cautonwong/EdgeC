#include "plugin_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>

#define MAX_PLUGINS 16

typedef struct {
    char name[64];
    void *handle;
    DriverOps ops;
} loaded_plugin_t;

static loaded_plugin_t g_plugins[MAX_PLUGINS];
static int g_plugin_count = 0;

int plugin_manager_init(const char *plugin_dir) {
    DIR *d;
    struct dirent *dir;
    d = opendir(plugin_dir);
    if (!d) {
        fprintf(stderr, "Cannot open plugin directory: %s\n", plugin_dir);
        return -1;
    }

    while ((dir = readdir(d)) != NULL) {
        // Look for .so files
        if (strstr(dir->d_name, ".so")) {
            if (g_plugin_count >= MAX_PLUGINS) {
                fprintf(stderr, "Max plugins reached (%d)\n", MAX_PLUGINS);
                break;
            }

            char fullpath[256];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", plugin_dir, dir->d_name);

            void *handle = dlopen(fullpath, RTLD_LAZY);
            if (!handle) {
                fprintf(stderr, "Failed to load plugin %s: %s\n", fullpath, dlerror());
                continue;
            }

            // Resolve the entry point symbol
            DriverOps* (*entry_point)(void);
            entry_point = dlsym(handle, "driver_entry_point");
            const char *dlsym_error = dlerror();
            if (dlsym_error) {
                fprintf(stderr, "Failed to find symbol 'driver_entry_point' in %s: %s\n", fullpath, dlsym_error);
                dlclose(handle);
                continue;
            }

            // Store the plugin info
            loaded_plugin_t *plugin = &g_plugins[g_plugin_count];
            plugin->handle = handle;
            plugin->ops = *entry_point();
            strncpy(plugin->name, plugin->ops.name, sizeof(plugin->name) - 1);
            
            printf("[Plugin] Loaded driver: %s\n", plugin->name);
            g_plugin_count++;
        }
    }
    closedir(d);
    return 0;
}

const DriverOps* plugin_manager_get_ops(const char *driver_name) {
    for (int i = 0; i < g_plugin_count; i++) {
        if (strcmp(g_plugins[i].name, driver_name) == 0) {
            return &g_plugins[i].ops;
        }
    }
    return NULL;
}

void plugin_manager_cleanup(void) {
    for (int i = 0; i < g_plugin_count; i++) {
        dlclose(g_plugins[i].handle);
    }
    g_plugin_count = 0;
}