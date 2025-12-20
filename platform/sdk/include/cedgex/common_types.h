#ifndef CEDGE_COMMON_TYPES_H
#define CEDGE_COMMON_TYPES_H

#include <stdint.h>
#include <time.h>

// --- Value Representation ---

typedef enum {
    CEDGEX_VAL_BOOL,
    CEDGEX_VAL_INT,
    CEDGEX_VAL_FLOAT,
    CEDGEX_VAL_STRING
} cedgex_value_type_t;

typedef union {
    int     b_val;
    int64_t i_val;
    double  f_val;
    char    *s_val;
} cedgex_value_t;


// --- Data Point - The Unified Data Model ---

typedef enum {
    PROTOCOL_UNKNOWN,
    PROTOCOL_MODBUS,
    PROTOCOL_IEC104,
    PROTOCOL_OPCUA,
    PROTOCOL_DLT645,
    PROTOCOL_SIMULATED // For testing
} cedgex_protocol_type_t;

typedef enum {
    QUALITY_GOOD = 0,
    QUALITY_BAD = 1
} cedgex_quality_t;

/**
 * @brief The unified data point structure, representing a single piece of
 * information from any device. This is the "common language" inside the framework.
 */
typedef struct {
    // Identity
    char key[64];         // A unique string identifier, e.g., "meter_01/voltage_a"

    // Source Information (for context and write-back)
    cedgex_protocol_type_t protocol;
    void *driver_ctx;     // Pointer to the specific driver instance context
    uintptr_t native_addr;  // The raw, original address (e.g., Modbus register 40001 or IEC104 IOA 100)

    // Value Information
    cedgex_value_type_t type;
    cedgex_value_t value;
    uint64_t source_ts;   // Timestamp from the device, if available
    uint64_t server_ts;   // Timestamp when the data was received by our framework
    cedgex_quality_t quality; // Data quality flag (e.g., from OPC UA or IEC 104)

} cedgex_datapoint_t;


// --- Legacy Structures (for compatibility with original example) ---
// These can be phased out as the project grows.

typedef struct {
    char resource_name[32];
    cedgex_value_type_t type;
    cedgex_value_t value;
} cedgex_reading_t;

typedef struct {
    char device_name[32];
    uint64_t timestamp;
    cedgex_reading_t reading;
} cedgex_event_t;


#endif // CEDGE_COMMON_TYPES_H
