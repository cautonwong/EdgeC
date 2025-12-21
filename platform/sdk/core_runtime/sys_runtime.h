#ifndef SYS_RUNTIME_H
#define SYS_RUNTIME_H

#include <uv.h>

/**
 * @brief Initializes and runs the main system event loop.
 * 
 * This function will take over the main thread, starting the libuv event loop
 * and running until the application is terminated. It should be the last
 * function called in main().
 * 
 * @param loop The main libuv loop.
 * @return 0 on successful shutdown.
 */
int sys_runtime_run(uv_loop_t *loop);


/**
 * @brief Sets up system-wide signal handlers for graceful shutdown.
 */
void sys_runtime_setup_signals(void);

#endif // SYS_RUNTIME_H
