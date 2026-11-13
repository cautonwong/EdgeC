/**
 * @file app_pressure.h
 * @brief Pure memory-pressure reclaim engine for the app platform.
 *
 * The engine reads the current pressure level and the reclaim candidate list
 * through an injected port. It owns no threads and no timers: the host calls
 * edge_pressure_step with a monotonic timestamp and the engine enforces the
 * step interval and cooldown itself.
 */
#ifndef EDGEOS_APP_PRESSURE_H
#define EDGEOS_APP_PRESSURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_PRESSURE_APP_ID_SIZE 64
#define APP_PRESSURE_MESSAGE_SIZE 160
#define APP_PRESSURE_MAX_CANDIDATES 64u
#define APP_PRESSURE_MAX_TRACKED_APPS 64u

/** Initial policy values; tune per target board. */
#define APP_PRESSURE_DEFAULT_WARN_PCT 70.0
#define APP_PRESSURE_DEFAULT_HIGH_PCT 85.0
#define APP_PRESSURE_DEFAULT_CRITICAL_PCT 95.0
#define APP_PRESSURE_DEFAULT_STEP_MS 3000u
#define APP_PRESSURE_DEFAULT_COOLDOWN_S 60u
#define APP_PRESSURE_DEFAULT_THRASH_COUNT 3u
#define APP_PRESSURE_DEFAULT_THRASH_WINDOW_S 3600u
#define APP_PRESSURE_DEFAULT_IDLE_MS 300000u

typedef enum {
    PRESSURE_LEVEL_NORMAL = 0,
    PRESSURE_LEVEL_WARN = 1,
    PRESSURE_LEVEL_HIGH = 2,
    PRESSURE_LEVEL_CRITICAL = 3,
} pressure_level_t;

typedef enum {
    RECLAIM_REASON_RECLAIMED = 0,
    RECLAIM_REASON_OOM_KILLED = 1,
    RECLAIM_REASON_BUDGET = 2,
} reclaim_reason_t;

typedef struct {
    double warn_pct;
    double high_pct;
    double critical_pct;
    uint32_t step_ms;
    uint32_t cooldown_s;
    uint32_t thrash_count;      /**< reclaims per window before an app is blocked */
    uint32_t thrash_window_s;
    uint64_t idle_ms;
    bool reclaim_enabled;       /**< false keeps monitoring and events, no suspend/stop */
} app_pressure_policy_t;

typedef struct {
    char app_id[APP_PRESSURE_APP_ID_SIZE];
    uint32_t priority; /**< 0..10; 0-3 background, 4-7 normal, 8-10 critical */
    bool keep_alive;   /**< core apps are never suspended or stopped */
    uint64_t last_active_ms;
    uint32_t rss_kb;
    bool desired_running;
    bool suspended;
} app_reclaim_candidate_t;

/**
 * Host port. `list_candidates` reports every app the platform knows about,
 * including suspended ones, so the engine can tell which to reactivate.
 */
typedef struct {
    void *ctx;
    int (*read_level)(void *ctx, pressure_level_t *out);
    int (*list_candidates)(void *ctx, app_reclaim_candidate_t *out, size_t max, size_t *count);
    int (*clear_cache)(void *ctx);
    int (*suspend)(void *ctx, const char *app_id);
    int (*stop)(void *ctx, const char *app_id, reclaim_reason_t reason);
    int (*activate)(void *ctx, const char *app_id);
    /** reason is RECLAIM_REASON_BUDGET for notices that are not a per-app reclaim. */
    void (*emit)(void *ctx, pressure_level_t level, const char *app_id,
                 reclaim_reason_t reason, const char *message);
} app_pressure_port_t;

typedef struct {
    char app_id[APP_PRESSURE_APP_ID_SIZE];
    uint64_t window_start_ms;
    uint32_t actions;
    bool blocked;
} app_pressure_thrash_t;

typedef struct {
    const app_pressure_port_t *port;
    app_pressure_policy_t policy;
    pressure_level_t level;
    bool has_stepped;
    uint64_t last_step_ms;
    bool has_acted;
    uint64_t last_action_ms;
    app_reclaim_candidate_t candidates[APP_PRESSURE_MAX_CANDIDATES];
    size_t candidate_count;
    app_pressure_thrash_t thrash[APP_PRESSURE_MAX_TRACKED_APPS];
    size_t thrash_count;
    uint32_t failure_count;
} app_pressure_t;

typedef enum {
    APP_PRESSURE_STEP_IDLE = 0,      /**< step interval has not elapsed */
    APP_PRESSURE_STEP_MONITORED = 1, /**< level read, no action taken */
    APP_PRESSURE_STEP_ACTED = 2,     /**< cache clear, activation, suspend, or stop */
} app_pressure_step_result_t;

/** Fill policy with the defaults above, reclaim enabled. */
void edge_pressure_policy_init(app_pressure_policy_t *policy);

/** Initialize an engine. A NULL policy uses edge_pressure_policy_init. */
void edge_pressure_init(app_pressure_t *pressure, const app_pressure_port_t *port,
                        const app_pressure_policy_t *policy);

/**
 * Run one monitoring step. now_ms is monotonic milliseconds. A step earlier
 * than step_ms after the previous one is skipped. On NORMAL, suspended apps
 * with desired_running are activated. On WARN, caches are cleared. On HIGH,
 * one idle, non-keep_alive app is suspended; on CRITICAL, one is stopped,
 * band by band background -> normal -> critical. Reclaim actions observe the
 * cooldown and are blocked per app after thrash_count reclaims in
 * thrash_window_s.
 */
app_pressure_step_result_t edge_pressure_step(app_pressure_t *pressure, uint64_t now_ms);

/** Record an OOM kill as an app failure (reclaimed actions are not failures). */
void edge_pressure_note_oom(app_pressure_t *pressure, const char *app_id);

uint32_t edge_pressure_failure_count(const app_pressure_t *pressure);

const char *edge_pressure_level_name(pressure_level_t level);

/**
 * Parse /proc/pressure/memory text ("some avg10=NN.NN ...") and map avg10
 * against the policy thresholds. Returns 0, or -1 when there is no usable
 * "some avg10=" line.
 */
int edge_pressure_parse_psi(const char *text, const app_pressure_policy_t *policy,
                            pressure_level_t *out);

/**
 * Parse /proc/meminfo MemTotal and MemAvailable and map the used percentage
 * against the policy thresholds. Returns 0, or -1 when either field is
 * missing or MemTotal is zero.
 */
int edge_pressure_parse_meminfo(const char *text, const app_pressure_policy_t *policy,
                                pressure_level_t *out);

/**
 * Prefer PSI, fall back to MemAvailable. Returns 0 on success, -1 when both
 * sources are absent or unparsable.
 */
int edge_pressure_level_from_sources(const char *psi_text, const char *meminfo_text,
                                     const app_pressure_policy_t *policy,
                                     pressure_level_t *out);

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_APP_PRESSURE_H */
