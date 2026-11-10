#ifndef EDGE_VERSION_H
#define EDGE_VERSION_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structured version information for the EdgeOS SDK.
 */
typedef struct {
    uint8_t major;           /**< Major version (breaking changes) */
    uint8_t minor;           /**< Minor version (new features) */
    uint8_t patch;           /**< Patch version (bug fixes) */
    const char* version_str; /**< Full version string (e.g., "4.5.0") */
    const char* git_hash;    /**< Git commit hash for traceability */
    const char* build_time;  /**< Timestamp of the build */
} edge_version_t;

/**
 * @brief Retrieves the SDK version information.
 * @return Pointer to a static, read-only version structure.
 */
const edge_version_t* edge_get_version(void);

/**
 * @brief Helper to retrieve the application root directory (based on /proc/self/exe).
 * @param out_path Buffer to store the path.
 * @param size Size of the buffer.
 * @return 0 on success, -1 on error.
 */
int edge_get_app_root(char* out_path, size_t size);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_VERSION_H