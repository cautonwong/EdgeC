/**
 * @file app_pressure.c
 * @brief Memory-pressure level mapping and reclaim decisions.
 */

#include "edgeos/app_pressure.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    RECLAIM_BAND_BACKGROUND = 0,
    RECLAIM_BAND_NORMAL = 1,
    RECLAIM_BAND_CRITICAL = 2,
    RECLAIM_BAND_COUNT = 3,
} reclaim_band_t;

void edge_pressure_policy_init(app_pressure_policy_t *policy) {
    if (!policy) return;
    *policy = (app_pressure_policy_t){
        .warn_pct = APP_PRESSURE_DEFAULT_WARN_PCT,
        .high_pct = APP_PRESSURE_DEFAULT_HIGH_PCT,
        .critical_pct = APP_PRESSURE_DEFAULT_CRITICAL_PCT,
        .step_ms = APP_PRESSURE_DEFAULT_STEP_MS,
        .cooldown_s = APP_PRESSURE_DEFAULT_COOLDOWN_S,
        .thrash_count = APP_PRESSURE_DEFAULT_THRASH_COUNT,
        .thrash_window_s = APP_PRESSURE_DEFAULT_THRASH_WINDOW_S,
        .idle_ms = APP_PRESSURE_DEFAULT_IDLE_MS,
        .reclaim_enabled = true,
    };
}

void edge_pressure_init(app_pressure_t *pressure, const app_pressure_port_t *port,
                        const app_pressure_policy_t *policy) {
    if (!pressure) return;
    memset(pressure, 0, sizeof(*pressure));
    pressure->port = port;
    if (policy) {
        pressure->policy = *policy;
    } else {
        edge_pressure_policy_init(&pressure->policy);
    }
    pressure->level = PRESSURE_LEVEL_NORMAL;
}

const char *edge_pressure_level_name(pressure_level_t level) {
    switch (level) {
    case PRESSURE_LEVEL_NORMAL:
        return "normal";
    case PRESSURE_LEVEL_WARN:
        return "warn";
    case PRESSURE_LEVEL_HIGH:
        return "high";
    case PRESSURE_LEVEL_CRITICAL:
        return "critical";
    default:
        return "unknown";
    }
}

static pressure_level_t level_for_pct(const app_pressure_policy_t *policy, double pct) {
    if (pct >= policy->critical_pct) return PRESSURE_LEVEL_CRITICAL;
    if (pct >= policy->high_pct) return PRESSURE_LEVEL_HIGH;
    if (pct >= policy->warn_pct) return PRESSURE_LEVEL_WARN;
    return PRESSURE_LEVEL_NORMAL;
}

int edge_pressure_parse_psi(const char *text, const app_pressure_policy_t *policy,
                            pressure_level_t *out) {
    if (!text || !policy || !out) return -1;
    for (const char *line = text; *line;) {
        double avg10 = 0.0;
        if (sscanf(line, "some avg10=%lf", &avg10) == 1) {
            if (!isfinite(avg10) || avg10 < 0.0) return -1;
            *out = level_for_pct(policy, avg10);
            return 0;
        }
        const char *newline = strchr(line, '\n');
        if (!newline) break;
        line = newline + 1;
    }
    return -1;
}

static int meminfo_kb(const char *text, const char *key, double *out) {
    const char *field = strstr(text, key);
    if (!field) return -1;
    double kb = 0.0;
    if (sscanf(field, "%*s %lf", &kb) != 1 || !isfinite(kb)) return -1;
    *out = kb;
    return 0;
}

int edge_pressure_parse_meminfo(const char *text, const app_pressure_policy_t *policy,
                                pressure_level_t *out) {
    if (!text || !policy || !out) return -1;
    double total = 0.0;
    double available = 0.0;
    if (meminfo_kb(text, "MemTotal:", &total) != 0) return -1;
    if (meminfo_kb(text, "MemAvailable:", &available) != 0) return -1;
    if (total <= 0.0 || available < 0.0) return -1;
    if (available > total) available = total;
    *out = level_for_pct(policy, (total - available) * 100.0 / total);
    return 0;
}

int edge_pressure_level_from_sources(const char *psi_text, const char *meminfo_text,
                                     const app_pressure_policy_t *policy,
                                     pressure_level_t *out) {
    if (!policy || !out) return -1;
    if (psi_text && edge_pressure_parse_psi(psi_text, policy, out) == 0) return 0;
    if (meminfo_text && edge_pressure_parse_meminfo(meminfo_text, policy, out) == 0) return 0;
    return -1;
}

static void emit_event(app_pressure_t *pressure, pressure_level_t level, const char *app_id,
                       reclaim_reason_t reason, const char *message) {
    if (!pressure->port || !pressure->port->emit) return;
    pressure->port->emit(pressure->port->ctx, level, app_id ? app_id : "", reason,
                         message ? message : "");
}

static bool cooldown_elapsed(const app_pressure_t *pressure, uint64_t now_ms) {
    if (!pressure->has_acted) return true;
    uint64_t cooldown_ms = (uint64_t)pressure->policy.cooldown_s * 1000u;
    if (now_ms < pressure->last_action_ms) return false;
    return now_ms - pressure->last_action_ms >= cooldown_ms;
}

static bool is_idle(const app_pressure_t *pressure, const app_reclaim_candidate_t *candidate,
                    uint64_t now_ms) {
    return now_ms >= candidate->last_active_ms &&
           now_ms - candidate->last_active_ms >= pressure->policy.idle_ms;
}

static reclaim_band_t band_of(const app_reclaim_candidate_t *candidate) {
    if (candidate->priority >= 8u) return RECLAIM_BAND_CRITICAL;
    if (candidate->priority >= 4u) return RECLAIM_BAND_NORMAL;
    return RECLAIM_BAND_BACKGROUND;
}

static bool load_candidates(app_pressure_t *pressure) {
    if (!pressure->port || !pressure->port->list_candidates) return false;
    size_t count = 0;
    if (pressure->port->list_candidates(pressure->port->ctx, pressure->candidates,
                                        APP_PRESSURE_MAX_CANDIDATES, &count) != 0)
        return false;
    pressure->candidate_count = count < (size_t)APP_PRESSURE_MAX_CANDIDATES
                                    ? count
                                    : (size_t)APP_PRESSURE_MAX_CANDIDATES;
    return true;
}

static app_pressure_thrash_t *thrash_find(app_pressure_t *pressure, const char *app_id) {
    for (size_t i = 0; i < pressure->thrash_count; i++) {
        if (strcmp(pressure->thrash[i].app_id, app_id) == 0) return &pressure->thrash[i];
    }
    return NULL;
}

static bool thrash_window_expired(const app_pressure_t *pressure,
                                  const app_pressure_thrash_t *entry, uint64_t now_ms) {
    uint64_t window_ms = (uint64_t)pressure->policy.thrash_window_s * 1000u;
    if (now_ms < entry->window_start_ms) return false;
    return now_ms - entry->window_start_ms >= window_ms;
}

static app_pressure_thrash_t *thrash_slot(app_pressure_t *pressure, const char *app_id,
                                          uint64_t now_ms) {
    app_pressure_thrash_t *entry = thrash_find(pressure, app_id);
    if (entry) return entry;

    size_t slot = pressure->thrash_count;
    if (slot < (size_t)APP_PRESSURE_MAX_TRACKED_APPS) {
        pressure->thrash_count++;
    } else {
        slot = 0;
        for (size_t i = 1; i < (size_t)APP_PRESSURE_MAX_TRACKED_APPS; i++) {
            if (pressure->thrash[i].window_start_ms < pressure->thrash[slot].window_start_ms)
                slot = i;
        }
    }

    entry = &pressure->thrash[slot];
    memset(entry, 0, sizeof(*entry));
    snprintf(entry->app_id, sizeof(entry->app_id), "%s", app_id);
    entry->window_start_ms = now_ms;
    return entry;
}

static bool thrash_blocked(app_pressure_t *pressure, const char *app_id, uint64_t now_ms) {
    app_pressure_thrash_t *entry = thrash_find(pressure, app_id);
    if (!entry || !entry->blocked) return false;
    if (!thrash_window_expired(pressure, entry, now_ms)) return true;
    entry->actions = 0;
    entry->blocked = false;
    entry->window_start_ms = now_ms;
    return false;
}

static void thrash_record(app_pressure_t *pressure, const char *app_id, uint64_t now_ms) {
    app_pressure_thrash_t *entry = thrash_slot(pressure, app_id, now_ms);
    if (thrash_window_expired(pressure, entry, now_ms)) {
        entry->window_start_ms = now_ms;
        entry->actions = 0;
        entry->blocked = false;
    }
    entry->actions++;
    if (pressure->policy.thrash_count == 0 || entry->actions < pressure->policy.thrash_count)
        return;
    if (entry->blocked) return;

    entry->blocked = true;
    char message[APP_PRESSURE_MESSAGE_SIZE];
    snprintf(message, sizeof(message), "thrash: %u reclaims in %u s", entry->actions,
             pressure->policy.thrash_window_s);
    emit_event(pressure, pressure->level, app_id, RECLAIM_REASON_RECLAIMED, message);
}

static const app_reclaim_candidate_t *pick_reclaim(app_pressure_t *pressure,
                                                   pressure_level_t level, uint64_t now_ms) {
    for (uint32_t band = 0; band < (uint32_t)RECLAIM_BAND_COUNT; band++) {
        const app_reclaim_candidate_t *best = NULL;
        for (size_t i = 0; i < pressure->candidate_count; i++) {
            const app_reclaim_candidate_t *candidate = &pressure->candidates[i];
            if (candidate->keep_alive || candidate->suspended) continue;
            if ((uint32_t)band_of(candidate) != band) continue;
            if (level == PRESSURE_LEVEL_HIGH && !is_idle(pressure, candidate, now_ms)) continue;
            if (thrash_blocked(pressure, candidate->app_id, now_ms)) continue;
            if (!best || candidate->rss_kb > best->rss_kb ||
                (candidate->rss_kb == best->rss_kb &&
                 strcmp(candidate->app_id, best->app_id) < 0))
                best = candidate;
        }
        if (best) return best;
    }
    return NULL;
}

static bool activate_desired(app_pressure_t *pressure) {
    bool acted = false;
    for (size_t i = 0; i < pressure->candidate_count; i++) {
        const app_reclaim_candidate_t *candidate = &pressure->candidates[i];
        if (!candidate->suspended || !candidate->desired_running) continue;
        if (!pressure->port || !pressure->port->activate) continue;
        if (pressure->port->activate(pressure->port->ctx, candidate->app_id) != 0) continue;
        emit_event(pressure, PRESSURE_LEVEL_NORMAL, candidate->app_id, RECLAIM_REASON_BUDGET,
                   "activated app");
        acted = true;
    }
    return acted;
}

app_pressure_step_result_t edge_pressure_step(app_pressure_t *pressure, uint64_t now_ms) {
    if (!pressure || !pressure->port || !pressure->port->read_level)
        return APP_PRESSURE_STEP_IDLE;

    if (pressure->has_stepped && now_ms >= pressure->last_step_ms &&
        now_ms - pressure->last_step_ms < pressure->policy.step_ms)
        return APP_PRESSURE_STEP_IDLE;
    pressure->has_stepped = true;
    pressure->last_step_ms = now_ms;

    pressure_level_t level;
    if (pressure->port->read_level(pressure->port->ctx, &level) != 0)
        return APP_PRESSURE_STEP_MONITORED;

    bool acted = false;
    if (level != pressure->level) {
        pressure->level = level;
        char message[APP_PRESSURE_MESSAGE_SIZE];
        snprintf(message, sizeof(message), "pressure level %s", edge_pressure_level_name(level));
        emit_event(pressure, level, "", RECLAIM_REASON_BUDGET, message);
    }

    switch (level) {
    case PRESSURE_LEVEL_NORMAL:
        if (load_candidates(pressure)) acted = activate_desired(pressure);
        break;
    case PRESSURE_LEVEL_WARN:
        if (pressure->port->clear_cache && pressure->port->clear_cache(pressure->port->ctx) == 0) {
            emit_event(pressure, level, "", RECLAIM_REASON_BUDGET, "cleared caches");
            acted = true;
        }
        break;
    case PRESSURE_LEVEL_HIGH:
    case PRESSURE_LEVEL_CRITICAL: {
        if (!pressure->policy.reclaim_enabled || !cooldown_elapsed(pressure, now_ms)) break;
        if (!load_candidates(pressure)) break;
        const app_reclaim_candidate_t *candidate = pick_reclaim(pressure, level, now_ms);
        if (!candidate) break;

        int rc;
        if (level == PRESSURE_LEVEL_HIGH) {
            rc = pressure->port->suspend
                     ? pressure->port->suspend(pressure->port->ctx, candidate->app_id)
                     : -1;
            if (rc == 0)
                emit_event(pressure, level, candidate->app_id, RECLAIM_REASON_RECLAIMED,
                           "suspended idle app");
        } else {
            rc = pressure->port->stop
                     ? pressure->port->stop(pressure->port->ctx, candidate->app_id,
                                            RECLAIM_REASON_RECLAIMED)
                     : -1;
            if (rc == 0)
                emit_event(pressure, level, candidate->app_id, RECLAIM_REASON_RECLAIMED,
                           "stopped app");
        }
        pressure->has_acted = true;
        pressure->last_action_ms = now_ms;
        if (rc == 0) {
            thrash_record(pressure, candidate->app_id, now_ms);
            acted = true;
        }
    } break;
    default:
        break;
    }

    return acted ? APP_PRESSURE_STEP_ACTED : APP_PRESSURE_STEP_MONITORED;
}

void edge_pressure_note_oom(app_pressure_t *pressure, const char *app_id) {
    if (!pressure) return;
    pressure->failure_count++;
    emit_event(pressure, pressure->level, app_id, RECLAIM_REASON_OOM_KILLED, "app oom_killed");
}

uint32_t edge_pressure_failure_count(const app_pressure_t *pressure) {
    return pressure ? pressure->failure_count : 0;
}
