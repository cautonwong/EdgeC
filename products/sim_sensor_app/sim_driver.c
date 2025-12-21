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

static int sim_read(cedgex_device_driver_t *self, cedgex_datapoint_t *out_point) {
    // Simulate generating data and populate the new DataPoint structure
    out_point->tag_id = 1001; // Example static tag_id
    out_point->ts = current_ts_ms();
    out_point->quality = CEDGEX_QUALITY_GOOD;
    
    out_point->type = CEDGEX_VAL_TYPE_FLOAT;
    // Generate a random number between 20.0 and 30.0
    out_point->val.f_val = 22.0 + ((float)rand() / (float)(RAND_MAX)) * 10.0;
    
    // The `key` and other source info would be mapped by the protocol adapter
    // This driver just provides the core data.
    
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
