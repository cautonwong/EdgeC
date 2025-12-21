#include "sys_runtime.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(_WIN32)
#include <execinfo.h> // For backtrace
#include <unistd.h>
#endif

// Generic signal handler
static void handle_signal(int sig) {
    fprintf(stderr, "\n[SysRuntime] Caught signal %d. Shutting down gracefully.\n", sig);

#if !defined(_WIN32)
    if (sig == SIGSEGV || sig == SIGILL || sig == SIGFPE || sig == SIGABRT) {
        void *array[20];
        size_t size = backtrace(array, 20);
        fprintf(stderr, "[SysRuntime] CRITICAL ERROR: signal %d:\n", sig);
        backtrace_symbols_fd(array, size, STDERR_FILENO);
    }
#endif
    
    // In a real application, you would stop the uv_loop here.
    // For now, we just exit.
    exit(1);
}

void sys_runtime_setup_signals(void) {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
#if !defined(_WIN32)
    signal(SIGSEGV, handle_signal);
    signal(SIGILL, handle_signal);
    signal(SIGFPE, handle_signal);
    signal(SIGABRT, handle_signal);
#endif
}

int sys_runtime_run(uv_loop_t *loop) {
    printf("[SysRuntime] Event loop started. Press Ctrl+C to exit.\n");
    return uv_run(loop, UV_RUN_DEFAULT);
}

