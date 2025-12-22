#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "driver_api.h"

/**
 * @brief Initializes the plugin manager.
 *
 * @param plugin_dir The directory where the .so driver plugins are located.
 * @return 0 on success, -1 on failure.
 */
int plugin_manager_init(const char *plugin_dir);

/**
 * @brief Gets the operations structure for a loaded driver.
 *
 * @param protocol_name The name of the protocol (e.g., "modbus-tcp").
 * @return A pointer to the DriverOps struct, or NULL if the driver is not
 * found.
 */
const DriverOps *plugin_manager_get_ops(const char *protocol_name);

/**
 * @brief Cleans up and unloads all plugins.
 */
void plugin_manager_cleanup(void);

#endif  // PLUGIN_MANAGER_H
