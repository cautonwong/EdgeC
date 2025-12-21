#ifndef TAG_MANAGER_H
#define TAG_MANAGER_H

#include <stdint.h>

/**
 * @brief Initializes the tag manager.
 * 
 * @param initial_capacity The initial capacity of the hash map.
 * @return 0 on success, -1 on failure.
 */
int tag_manager_init(size_t initial_capacity);

/**
 * @brief Frees all resources used by the tag manager.
 */
void tag_manager_cleanup(void);

/**
 * @brief Gets the integer ID for a given tag name string.
 * 
 * If the tag name does not exist, it will be added to the map and a new
 * unique ID will be created and returned.
 * 
 * @param tag_name The string name of the tag (e.g., "pcs_01/active_power").
 * @return The unique uint32_t ID for the tag. Returns 0 on failure.
 */
uint32_t tag_manager_get_id(const char *tag_name);

/**
 * @brief Gets the string name for a given tag ID.
 * 
 * This is less frequently used but can be useful for debugging.
 * 
 * @param tag_id The ID of the tag.
 * @return A pointer to the string name, or NULL if not found. The string is
 *         owned by the tag manager and should not be freed.
 */
const char* tag_manager_get_name(uint32_t tag_id);

#endif // TAG_MANAGER_H
