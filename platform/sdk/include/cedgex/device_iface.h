#ifndef CEDGE_DEVICE_IFACE_H
#define CEDGE_DEVICE_IFACE_H

#include <cedgex/common_types.h>

// Forward declaration
typedef struct cedgex_device_driver_t cedgex_device_driver_t;

// Defines the virtual function table (vtable) for a device driver.
// This is the key to achieving polymorphism in C.
typedef struct {
    int (*init)(cedgex_device_driver_t *self);
    int (*read)(cedgex_device_driver_t *self, cedgex_event_t *out_event);
    int (*close)(cedgex_device_driver_t *self);
} cedgex_device_vtable_t;

// The "base class" structure for a device driver.
struct cedgex_device_driver_t {
    const cedgex_device_vtable_t *vtable; // Pointer to the virtual function table
    char name[32];                        // Name of the device instance
    void *private_data;                   // For implementation-specific data (e.g., a Modbus context or a file descriptor)
};

// Helper macros for easy virtual function calls (syntactic sugar)
#define DEVICE_INIT(d)       ((d)->vtable->init(d))
#define DEVICE_READ(d, e)    ((d)->vtable->read(d, e))
#define DEVICE_CLOSE(d)      ((d)->vtable->close(d))

#endif // CEDGE_DEVICE_IFACE_H
