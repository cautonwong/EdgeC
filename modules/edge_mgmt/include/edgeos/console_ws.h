#ifndef EDGEOS_CONSOLE_WS_H
#define EDGEOS_CONSOLE_WS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONSOLE_WS_MAX_CMD_LEN    512
#define CONSOLE_WS_MAX_RESP_LEN   8192
#define CONSOLE_WS_MAX_COMPLETIONS 32

/**
 * Command execution result structure
 */
typedef struct {
    int status_code;             /* 0 on success, non-zero on error */
    char output[CONSOLE_WS_MAX_RESP_LEN];
    uint64_t timestamp_ns;       /* MTT timestamp */
} console_cmd_result_t;

/**
 * Initialize Web Console WS / CLI Bridge
 */
int console_ws_init(void);

/**
 * Dispatch and execute an EdgeOS CLI command.
 * Output is formatted with ANSI SGR color codes.
 *
 * @param cmd The command line string
 * @param out_result Pointer to result structure
 * @return 0 on success, negative error code on failure
 */
int console_ws_dispatch_command(const char *cmd, console_cmd_result_t *out_result);

/**
 * Query candidate completions for the given prefix.
 *
 * @param prefix The input prefix string
 * @param out_completions Array of char pointers to store matching commands
 * @param max_items Maximum number of completions to return
 * @param out_count Actual number of completions found
 * @return 0 on success, negative error code on failure
 */
int console_ws_get_completions(const char *prefix, 
                               char out_completions[CONSOLE_WS_MAX_COMPLETIONS][64], 
                               size_t max_items, 
                               size_t *out_count);

/**
 * Format raw string into ANSI colored envelope JSON.
 */
int console_ws_format_json_response(const console_cmd_result_t *res, char *json_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_CONSOLE_WS_H */
