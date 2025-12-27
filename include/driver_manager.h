#pragma once

#include <stdint.h>

#include "arbiter.h"  // For arbiter_bus_t

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for the driver manager
typedef struct driver_manager_s driver_manager_t;

/**
 * @brief Creates and initializes the driver manager.
 *        This function will attempt to open the physical devices (e.g., serial ports)
 *        and prepare them for use.
 *
 * @return A pointer to the driver manager instance, or NULL on failure.
 */
driver_manager_t* driver_manager_create(void);

/**
 * @brief Destroys the driver manager and closes all opened file descriptors.
 *
 * @param dm The driver manager instance to destroy.
 */
void driver_manager_destroy(driver_manager_t* dm);

/**
 * @brief Retrieves the file descriptor for a specific bus.
 *
 * @param dm The driver manager instance.
 * @param bus The bus identifier.
 * @return The file descriptor (>= 0) on success, or -1 if the bus is not found or not available.
 */
int driver_manager_get_fd(driver_manager_t* dm, arbiter_bus_t bus);

#ifdef __cplusplus
}
#endif
