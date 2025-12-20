#include <cedgex/device_iface.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Forward declaration of the factory function
cedgex_device_driver_t* create_sim_device(const char* name);

// --- Private Implementation ---

static int sim_init(cedgex_device_driver_t *self) {
    printf("[Driver] Initializing %s...\n", self->name);
    // In a real driver, you might initialize hardware or allocate private data here.
    // srand() is used for the simulation, seeded by the current time.
    srand(time(NULL));
    return 0; // Success
}

static int sim_read(cedgex_device_driver_t *self, cedgex_event_t *out_event) {
    // Simulate generating data
    strncpy(out_event->device_name, self->name, 31);
    out_event->device_name[31] = '\0';
    out_event->timestamp = (uint64_t)time(NULL);
    
    out_event->reading.type = VAL_FLOAT;
    strcpy(out_event->reading.resource_name, "temperature");
    // Generate a random number between 20.0 and 30.0
    out_event->reading.value.f_val = 20.0 + ((float)rand() / (float)(RAND_MAX)) * 10.0;
    
    return 0; // Success
}

static int sim_close(cedgex_device_driver_t *self) {
    printf("[Driver] Closing %s\n", self->name);
    // Free any resources allocated in `priv_data` here.
    if (self->private_data) {
        free(self->private_data);
        self->private_data = NULL;
    }
    return 0;
}

// --- Public Interface ---

// The single, static instance of the virtual function table for this driver type.
static const cedgex_device_vtable_t sim_vtable = {
    .init = sim_init,
    .read = sim_read,
    .close = sim_close,
};

// Factory function: creates an instance of the simulated device driver.
cedgex_device_driver_t* create_sim_device(const char* name) {
    cedgex_device_driver_t *dev = (cedgex_device_driver_t*)malloc(sizeof(cedgex_device_driver_t));
    if (!dev) {
        return NULL;
    }
    
    strncpy(dev->name, name, 31);
    dev->name[31] = '\0';
    dev->vtable = &sim_vtable; // Bind the vtable to this instance
    dev->private_data = NULL;  // No private data for this simple driver
    
    return dev;
}
