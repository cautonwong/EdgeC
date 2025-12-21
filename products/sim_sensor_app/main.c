#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <cedgex/device_iface.h> // Include from our SDK

// --- Signal Handling for graceful shutdown & debugging ---
#if !defined(_WIN32)
#include <execinfo.h> // For backtrace
#include <unistd.h>
#endif

void handle_signal(int sig) {
    fprintf(stderr, "\nCaught signal %d. Shutting down.\n", sig);

#if !defined(_WIN32)
    if (sig == SIGSEGV || sig == SIGILL || sig == SIGFPE) {
        void *array[10];
        size_t size;
        fprintf(stderr, "Error: signal %d:\n", sig);
        size = backtrace(array, 10);
        backtrace_symbols_fd(array, size, STDERR_FILENO);
    }
#endif

    // In a real application with a running libuv loop,
    // you would use this to gracefully stop the loop.
    // For this simple example, we just exit.
    exit(1);
}

// --- Application Logic ---

// Forward declare the factory function from our driver implementation
extern cedgex_device_driver_t* create_sim_device(const char* name);

// A simple wrapper to pass context to the libuv timer callback
typedef struct {
    cedgex_device_driver_t *device;
} timer_context_t;

// This function is called by the libuv timer on a regular interval
void on_timer_tick(uv_timer_t *handle) {
    timer_context_t *ctx = (timer_context_t*)handle->data;
    cedgex_datapoint_t point;
    
    // Use the SDK's generic DEVICE_READ macro to read from the device
    if (DEVICE_READ(ctx->device, &point) == 0) {
        printf("[App] DataPoint: ts=%llu, tag_id=%u, quality=%d, value=%.2f\n", 
               (unsigned long long)point.ts, 
               point.tag_id, 
               point.quality,
               point.val.f_val);
        
        // TODO: Here you would push the 'point' object to the data pipeline.
    }
}

int main() {
    // Setup signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
#if !defined(_WIN32)
    signal(SIGSEGV, handle_signal);
    signal(SIGILL, handle_signal);
    signal(SIGFPE, handle_signal);
#endif

    // Get the default libuv event loop
    uv_loop_t *loop = uv_default_loop();

    // 1. Create a device instance using its factory function
    cedgex_device_driver_t *sensor1 = create_sim_device("Sensor-001");
    if (!sensor1) {
        fprintf(stderr, "Failed to create device.\n");
        return 1;
    }

    // 2. Initialize the device using the generic SDK macro
    DEVICE_INIT(sensor1);

    // 3. Set up a libuv timer to poll the device every 2 seconds
    uv_timer_t timer_req;
    timer_context_t ctx = { .device = sensor1 };
    
    uv_timer_init(loop, &timer_req);
    timer_req.data = &ctx; // Pass context to the callback
    uv_timer_start(&timer_req, on_timer_tick, 0, 2000);

    printf("C-EdgeX Application Started. Press Ctrl+C to exit.\n");
    
    // 4. Start the libuv event loop. This will block until the loop is stopped.
    uv_run(loop, UV_RUN_DEFAULT);

    // --- Cleanup (code here will run after uv_run returns) ---
    printf("Cleaning up resources...\n");
    DEVICE_CLOSE(sensor1);
    free(sensor1);
    
    // This is important to allow libuv to clean up its handles
    uv_loop_close(loop); 

    return 0;
}
