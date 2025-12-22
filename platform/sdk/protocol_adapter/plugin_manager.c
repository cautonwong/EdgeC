#include "plugin_manager.h"

#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PLUGINS 16

typedef struct {
  char name[64];
  void *handle;
  DriverOps *ops;
} LoadedPlugin;

static LoadedPlugin g_plugins[MAX_PLUGINS];
static int g_plugin_count = 0;

int plugin_manager_init(const char *plugin_dir) {
  DIR *d;
  struct dirent *dir;
  d = opendir(plugin_dir);
  if (!d) {
    perror("[PluginManager] Failed to open plugin directory");
    return -1;
  }

  printf("[PluginManager] Scanning for plugins in %s...\n", plugin_dir);

  while ((dir = readdir(d)) != NULL && g_plugin_count < MAX_PLUGINS) {
    // Look for files ending in .so
    if (strstr(dir->d_name, ".so")) {
      char full_path[256];
      snprintf(full_path, sizeof(full_path), "%s/%s", plugin_dir, dir->d_name);

      void *handle = dlopen(full_path, RTLD_NOW | RTLD_LOCAL);
      if (!handle) {
        fprintf(stderr, "[PluginManager] Failed to load plugin %s: %s\n",
                full_path, dlerror());
        continue;
      }

      // Find the entry point function
      DriverEntryPointFunc entry_point =
          (DriverEntryPointFunc)dlsym(handle, DRIVER_ENTRY_POINT);
      if (!entry_point) {
        fprintf(stderr, "[PluginManager] Failed to find entry point in %s\n",
                full_path);
        dlclose(handle);
        continue;
      }

      DriverOps *ops = entry_point();
      if (!ops || !ops->name) {
        fprintf(stderr,
                "[PluginManager] Plugin %s returned invalid ops struct\n",
                full_path);
        dlclose(handle);
        continue;
      }

      // Store the loaded plugin
      LoadedPlugin *p = &g_plugins[g_plugin_count++];
      strncpy(p->name, ops->name, sizeof(p->name) - 1);
      p->handle = handle;
      p->ops = ops;

      printf("[PluginManager] Successfully loaded driver: %s\n", p->name);
    }
  }

  closedir(d);
  return 0;
}

const DriverOps *plugin_manager_get_ops(const char *protocol_name) {
  for (int i = 0; i < g_plugin_count; i++) {
    if (strcmp(g_plugins[i].name, protocol_name) == 0) {
      return g_plugins[i].ops;
    }
  }
  return NULL;
}

void plugin_manager_cleanup(void) {
  printf("[PluginManager] Unloading all plugins...\n");
  for (int i = 0; i < g_plugin_count; i++) {
    if (g_plugins[i].handle) {
      dlclose(g_plugins[i].handle);
    }
  }
  g_plugin_count = 0;
}
