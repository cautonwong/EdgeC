#ifndef EDGEOS_CONSOLE_AUDIT_H
#define EDGEOS_CONSOLE_AUDIT_H

#include "edgeos/console_rbac.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONSOLE_AUDIT_MAX_ENTRIES 1024
#define CONSOLE_AUDIT_CMD_LEN     512
#define CONSOLE_AUDIT_RESP_LEN    256

/**
 * Immutable Audit Record for Industrial Web Console Commands
 */
typedef struct {
    uint64_t id;                       /* Monotonic serial ID */
    uint64_t timestamp_ns;             /* Wall-clock MTT timestamp in nanoseconds */
    char username[64];                 /* Operator username */
    console_role_t role;               /* User role */
    char client_ip[48];                /* Remote client IP */
    char command[CONSOLE_AUDIT_CMD_LEN]; /* Executed command string */
    console_risk_level_t risk_level;   /* Evaluated risk level */
    bool two_man_verified;             /* Was Two-Man Rule verified */
    char supervisor_user[64];          /* Approving supervisor (if high risk) */
    int execution_status;              /* 0 for success, non-zero for error/blocked */
    char response_summary[CONSOLE_AUDIT_RESP_LEN]; /* Execution summary */
    uint32_t checksum_crc32;           /* CRC32 tamper-evidence seal */
} console_audit_entry_t;

/**
 * Initialize Audit Trail Subsystem
 *
 * @param db_file Path to persistent audit log file (or NULL for in-memory)
 * @return 0 on success, negative error code on failure
 */
int console_audit_init(const char *db_file);

/**
 * Append an immutable audit entry
 */
int console_audit_log(const console_audit_entry_t *entry);

/**
 * Record a command execution with automatic CRC32 calculation and timestamping
 */
int console_audit_record(const char *username, 
                         console_role_t role, 
                         const char *client_ip,
                         const char *command, 
                         console_risk_level_t risk_level,
                         bool two_man_verified, 
                         const char *supervisor_user,
                         int execution_status, 
                         const char *response_summary);

/**
 * Query audit records within a given time range and optional filters
 */
int console_audit_query(uint64_t start_time_ns, 
                        uint64_t end_time_ns, 
                        console_role_t role_filter, 
                        const char *user_filter,
                        console_audit_entry_t *out_entries, 
                        size_t max_entries, 
                        size_t *out_count);

/**
 * Export audit logs as a JSON array string
 */
int console_audit_export_json(const char *user_filter, char *out_json, size_t max_len);

/**
 * Verify cryptographic / CRC32 integrity of all stored audit records
 *
 * @param out_valid_count Pointer to store count of valid tamper-free entries
 * @param out_tampered_count Pointer to store count of tampered entries
 * @return 0 if all entries valid, -1 if tampering detected
 */
int console_audit_verify_integrity(size_t *out_valid_count, size_t *out_tampered_count);

/**
 * Get total number of audit records stored
 */
size_t console_audit_get_total_count(void);

/**
 * Clear or reset audit subsystem (for testing)
 */
void console_audit_reset(void);

/**
 * Close and flush audit subsystem
 */
void console_audit_close(void);

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_CONSOLE_AUDIT_H */
