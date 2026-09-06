#include "edgeos/console_audit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

static console_audit_entry_t s_audit_entries[CONSOLE_AUDIT_MAX_ENTRIES];
static size_t s_audit_count = 0;
static uint64_t s_next_id = 1;
static pthread_mutex_t s_audit_lock = PTHREAD_MUTEX_INITIALIZER;
static bool s_audit_initialized = false;
static char s_audit_file[256] = {0};

/* Standard IEEE 802.3 CRC32 Implementation */
static uint32_t calc_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (size_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return ~crc;
}

static uint32_t compute_entry_crc32(const console_audit_entry_t *e)
{
    /* Calculate CRC over all fields except checksum_crc32 itself */
    size_t payload_len = offsetof(console_audit_entry_t, checksum_crc32);
    return calc_crc32((const uint8_t*)e, payload_len);
}

static uint64_t get_current_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}

int console_audit_init(const char *db_file)
{
    pthread_mutex_lock(&s_audit_lock);
    if (db_file && *db_file) {
        strncpy(s_audit_file, db_file, sizeof(s_audit_file) - 1);
    } else {
        s_audit_file[0] = '\0';
    }
    s_audit_initialized = true;
    pthread_mutex_unlock(&s_audit_lock);
    return 0;
}

int console_audit_log(const console_audit_entry_t *entry)
{
    if (!entry) return -EINVAL;

    pthread_mutex_lock(&s_audit_lock);
    if (!s_audit_initialized) {
        s_audit_initialized = true;
    }

    size_t idx;
    if (s_audit_count < CONSOLE_AUDIT_MAX_ENTRIES) {
        idx = s_audit_count;
        s_audit_count++;
    } else {
        /* Ring shift or overwrite oldest */
        memmove(&s_audit_entries[0], &s_audit_entries[1], sizeof(console_audit_entry_t) * (CONSOLE_AUDIT_MAX_ENTRIES - 1));
        idx = CONSOLE_AUDIT_MAX_ENTRIES - 1;
    }

    memcpy(&s_audit_entries[idx], entry, sizeof(console_audit_entry_t));
    s_audit_entries[idx].id = s_next_id++;
    if (s_audit_entries[idx].timestamp_ns == 0) {
        s_audit_entries[idx].timestamp_ns = get_current_time_ns();
    }
    s_audit_entries[idx].checksum_crc32 = compute_entry_crc32(&s_audit_entries[idx]);

    /* If file persistence configured, write line */
    if (s_audit_file[0] != '\0') {
        FILE *fp = fopen(s_audit_file, "a");
        if (fp) {
            fprintf(fp, "%lu,%lu,%s,%d,%s,\"%s\",%d,%d,%s,%d,\"%s\",%08X\n",
                    (unsigned long)s_audit_entries[idx].id,
                    (unsigned long)s_audit_entries[idx].timestamp_ns,
                    s_audit_entries[idx].username,
                    (int)s_audit_entries[idx].role,
                    s_audit_entries[idx].client_ip,
                    s_audit_entries[idx].command,
                    (int)s_audit_entries[idx].risk_level,
                    s_audit_entries[idx].two_man_verified ? 1 : 0,
                    s_audit_entries[idx].supervisor_user,
                    s_audit_entries[idx].execution_status,
                    s_audit_entries[idx].response_summary,
                    s_audit_entries[idx].checksum_crc32);
            fclose(fp);
        }
    }

    pthread_mutex_unlock(&s_audit_lock);
    return 0;
}

int console_audit_record(const char *username, 
                         console_role_t role, 
                         const char *client_ip,
                         const char *command, 
                         console_risk_level_t risk_level,
                         bool two_man_verified, 
                         const char *supervisor_user,
                         int execution_status, 
                         const char *response_summary)
{
    console_audit_entry_t entry;
    memset(&entry, 0, sizeof(entry));

    if (username) strncpy(entry.username, username, sizeof(entry.username) - 1);
    entry.role = role;
    if (client_ip) strncpy(entry.client_ip, client_ip, sizeof(entry.client_ip) - 1);
    if (command) strncpy(entry.command, command, sizeof(entry.command) - 1);
    entry.risk_level = risk_level;
    entry.two_man_verified = two_man_verified;
    if (supervisor_user) strncpy(entry.supervisor_user, supervisor_user, sizeof(entry.supervisor_user) - 1);
    entry.execution_status = execution_status;
    if (response_summary) strncpy(entry.response_summary, response_summary, sizeof(entry.response_summary) - 1);

    return console_audit_log(&entry);
}

int console_audit_query(uint64_t start_time_ns, 
                        uint64_t end_time_ns, 
                        console_role_t role_filter, 
                        const char *user_filter,
                        console_audit_entry_t *out_entries, 
                        size_t max_entries, 
                        size_t *out_count)
{
    if (!out_entries || !out_count || max_entries == 0) return -EINVAL;

    pthread_mutex_lock(&s_audit_lock);
    size_t matches = 0;

    for (size_t i = 0; i < s_audit_count && matches < max_entries; i++) {
        const console_audit_entry_t *e = &s_audit_entries[i];

        if (start_time_ns > 0 && e->timestamp_ns < start_time_ns) continue;
        if (end_time_ns > 0 && e->timestamp_ns > end_time_ns) continue;
        if (role_filter > 0 && e->role != role_filter) continue;
        if (user_filter && *user_filter && strcasecmp(e->username, user_filter) != 0) continue;

        memcpy(&out_entries[matches], e, sizeof(console_audit_entry_t));
        matches++;
    }

    *out_count = matches;
    pthread_mutex_unlock(&s_audit_lock);
    return 0;
}

int console_audit_export_json(const char *user_filter, char *out_json, size_t max_len)
{
    if (!out_json || max_len < 16) return -EINVAL;

    pthread_mutex_lock(&s_audit_lock);
    size_t offset = 0;
    int written = snprintf(out_json + offset, max_len - offset, "{\"total\":%zu,\"records\":[", s_audit_count);
    if (written > 0) offset += (size_t)written;

    bool first = true;
    for (size_t i = 0; i < s_audit_count && offset < max_len - 64; i++) {
        const console_audit_entry_t *e = &s_audit_entries[i];
        if (user_filter && *user_filter && strcasecmp(e->username, user_filter) != 0) {
            continue;
        }

        if (!first) {
            int sep = snprintf(out_json + offset, max_len - offset, ",");
            if (sep > 0) offset += (size_t)sep;
        }
        first = false;

        int n = snprintf(out_json + offset, max_len - offset,
                         "{\"id\":%lu,\"ts\":%lu,\"user\":\"%s\",\"role\":\"%s\",\"ip\":\"%s\","
                         "\"cmd\":\"%s\",\"risk\":%d,\"two_man\":%s,\"sup\":\"%s\","
                         "\"status\":%d,\"summary\":\"%s\",\"crc\":\"0x%08X\"}",
                         (unsigned long)e->id,
                         (unsigned long)e->timestamp_ns,
                         e->username,
                         console_rbac_role_to_str(e->role),
                         e->client_ip,
                         e->command,
                         (int)e->risk_level,
                         e->two_man_verified ? "true" : "false",
                         e->supervisor_user,
                         e->execution_status,
                         e->response_summary,
                         e->checksum_crc32);
        if (n > 0) offset += (size_t)n;
    }

    if (offset < max_len - 3) {
        snprintf(out_json + offset, max_len - offset, "]}");
    }

    pthread_mutex_unlock(&s_audit_lock);
    return 0;
}

int console_audit_verify_integrity(size_t *out_valid_count, size_t *out_tampered_count)
{
    pthread_mutex_lock(&s_audit_lock);
    size_t valid = 0;
    size_t tampered = 0;

    for (size_t i = 0; i < s_audit_count; i++) {
        uint32_t expected = compute_entry_crc32(&s_audit_entries[i]);
        if (expected == s_audit_entries[i].checksum_crc32) {
            valid++;
        } else {
            tampered++;
        }
    }

    if (out_valid_count) *out_valid_count = valid;
    if (out_tampered_count) *out_tampered_count = tampered;

    pthread_mutex_unlock(&s_audit_lock);
    return (tampered == 0) ? 0 : -1;
}

size_t console_audit_get_total_count(void)
{
    pthread_mutex_lock(&s_audit_lock);
    size_t c = s_audit_count;
    pthread_mutex_unlock(&s_audit_lock);
    return c;
}

void console_audit_reset(void)
{
    pthread_mutex_lock(&s_audit_lock);
    s_audit_count = 0;
    s_next_id = 1;
    pthread_mutex_unlock(&s_audit_lock);
}

void console_audit_close(void)
{
    pthread_mutex_lock(&s_audit_lock);
    s_audit_initialized = false;
    pthread_mutex_unlock(&s_audit_lock);
}
