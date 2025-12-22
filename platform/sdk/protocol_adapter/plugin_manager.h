#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <cedgex/driver_api.h>

/**
 * @brief Initializes the plugin manager.
 * 
 * @param plugin_dir The directory where the .so plugin files are located.
 * @return 0 on success, -1 on error.
 */
int plugin_manager_init(const char *plugin_dir);

/**
 * @brief Gets the operations table for a loaded driver.
 * 
 * @param driver_name The name of the driver (e.g., "modbus_tcp").
 * @return A pointer to the DriverOps structure, or NULL if not found.
 */
const DriverOps* plugin_manager_get_ops(const char *driver_name);

/**
 * @brief Unloads all plugins and cleans up the manager.
 */
void plugin_manager_cleanup(void);

#endif /* PLUGIN_MANAGER_H */