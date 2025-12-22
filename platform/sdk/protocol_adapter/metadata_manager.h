#ifndef METADATA_MANAGER_H
#define METADATA_MANAGER_H

// This module is responsible for parsing device profiles (JSON),
// managing the mapping between tag names and tag IDs, and providing
// drivers with their specific configuration.

#include <stddef.h>

/**
 * @brief Initializes the metadata manager.
 * @return 0 on success.
 */
int metadata_manager_init(void);

/**
 * @brief Loads a device profile from a JSON string.
 * @param json_config The configuration to load.
 * @return 0 on success, -1 on failure.
 */
int metadata_manager_load_profile(const char *json_config);

/**
 * @brief Cleans up all metadata resources.
 */
void metadata_manager_cleanup(void);

#endif  // METADATA_MANAGER_H
