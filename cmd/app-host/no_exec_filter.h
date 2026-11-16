#ifndef APP_HOST_NO_EXEC_FILTER_H
#define APP_HOST_NO_EXEC_FILTER_H

/**
 * Install a seccomp filter that denies execve/execveat.
 *
 * Returns 0 on success, -1 with errno set when seccomp is unavailable or the
 * architecture is not covered. Callers may continue without it; the launcher
 * already applies its own filter.
 */
int app_host_install_no_exec_filter(void);

#endif
