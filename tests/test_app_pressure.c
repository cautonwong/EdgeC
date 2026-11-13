#include <cmocka.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "edgeos/app_pressure.h"

#define FAKE_MAX_ACTION_APPS 8
#define FAKE_MAX_EVENTS 32

typedef struct {
    pressure_level_t level;
    app_reclaim_candidate_t candidates[APP_PRESSURE_MAX_CANDIDATES];
    size_t candidate_count;
    bool remove_on_stop;
    int read_calls;
    int list_calls;
    int clear_calls;
    int suspend_calls;
    int stop_calls;
    int activate_calls;
    char suspended[FAKE_MAX_ACTION_APPS][APP_PRESSURE_APP_ID_SIZE];
    char stopped[FAKE_MAX_ACTION_APPS][APP_PRESSURE_APP_ID_SIZE];
    reclaim_reason_t stop_reasons[FAKE_MAX_ACTION_APPS];
    char activated[FAKE_MAX_ACTION_APPS][APP_PRESSURE_APP_ID_SIZE];
    struct {
        pressure_level_t level;
        char app_id[APP_PRESSURE_APP_ID_SIZE];
        reclaim_reason_t reason;
        char message[APP_PRESSURE_MESSAGE_SIZE];
    } events[FAKE_MAX_EVENTS];
    size_t event_count;
} fake_port_t;

static int fake_read_level(void *ctx, pressure_level_t *out) {
    fake_port_t *fake = ctx;
    fake->read_calls++;
    *out = fake->level;
    return 0;
}

static int fake_list_candidates(void *ctx, app_reclaim_candidate_t *out, size_t max,
                                size_t *count) {
    fake_port_t *fake = ctx;
    fake->list_calls++;
    size_t n = fake->candidate_count < max ? fake->candidate_count : max;
    memcpy(out, fake->candidates, n * sizeof(out[0]));
    *count = n;
    return 0;
}

static int fake_clear_cache(void *ctx) {
    fake_port_t *fake = ctx;
    fake->clear_calls++;
    return 0;
}

static void fake_set_suspended(fake_port_t *fake, const char *app_id, bool suspended) {
    for (size_t i = 0; i < fake->candidate_count; i++) {
        if (strcmp(fake->candidates[i].app_id, app_id) == 0)
            fake->candidates[i].suspended = suspended;
    }
}

static int fake_suspend(void *ctx, const char *app_id) {
    fake_port_t *fake = ctx;
    if (fake->suspend_calls < FAKE_MAX_ACTION_APPS)
        snprintf(fake->suspended[fake->suspend_calls], APP_PRESSURE_APP_ID_SIZE, "%s", app_id);
    fake->suspend_calls++;
    fake_set_suspended(fake, app_id, true);
    return 0;
}

static int fake_stop(void *ctx, const char *app_id, reclaim_reason_t reason) {
    fake_port_t *fake = ctx;
    if (fake->stop_calls < FAKE_MAX_ACTION_APPS) {
        snprintf(fake->stopped[fake->stop_calls], APP_PRESSURE_APP_ID_SIZE, "%s", app_id);
        fake->stop_reasons[fake->stop_calls] = reason;
    }
    fake->stop_calls++;
    if (fake->remove_on_stop) {
        for (size_t i = 0; i < fake->candidate_count; i++) {
            if (strcmp(fake->candidates[i].app_id, app_id) == 0) {
                memmove(&fake->candidates[i], &fake->candidates[i + 1],
                        (fake->candidate_count - i - 1) * sizeof(fake->candidates[0]));
                fake->candidate_count--;
                break;
            }
        }
    }
    return 0;
}

static int fake_activate(void *ctx, const char *app_id) {
    fake_port_t *fake = ctx;
    if (fake->activate_calls < FAKE_MAX_ACTION_APPS)
        snprintf(fake->activated[fake->activate_calls], APP_PRESSURE_APP_ID_SIZE, "%s", app_id);
    fake->activate_calls++;
    fake_set_suspended(fake, app_id, false);
    return 0;
}

static void fake_emit(void *ctx, pressure_level_t level, const char *app_id,
                      reclaim_reason_t reason, const char *message) {
    fake_port_t *fake = ctx;
    if (fake->event_count >= FAKE_MAX_EVENTS) return;
    size_t i = fake->event_count++;
    fake->events[i].level = level;
    fake->events[i].reason = reason;
    snprintf(fake->events[i].app_id, APP_PRESSURE_APP_ID_SIZE, "%s", app_id ? app_id : "");
    snprintf(fake->events[i].message, APP_PRESSURE_MESSAGE_SIZE, "%s", message ? message : "");
}

static void fake_init(fake_port_t *fake, pressure_level_t level) {
    memset(fake, 0, sizeof(*fake));
    fake->level = level;
    fake->remove_on_stop = true;
}

static app_pressure_port_t fake_port(fake_port_t *fake) {
    app_pressure_port_t port = {
        .ctx = fake,
        .read_level = fake_read_level,
        .list_candidates = fake_list_candidates,
        .clear_cache = fake_clear_cache,
        .suspend = fake_suspend,
        .stop = fake_stop,
        .activate = fake_activate,
        .emit = fake_emit,
    };
    return port;
}

static bool fake_has_event(const fake_port_t *fake, const char *app_id, reclaim_reason_t reason,
                           const char *message_part) {
    for (size_t i = 0; i < fake->event_count; i++) {
        if (app_id && strcmp(fake->events[i].app_id, app_id) != 0) continue;
        if (fake->events[i].reason != reason) continue;
        if (message_part && !strstr(fake->events[i].message, message_part)) continue;
        return true;
    }
    return false;
}

static app_reclaim_candidate_t candidate(const char *app_id, uint32_t priority, uint32_t rss_kb,
                                         bool keep_alive, uint64_t last_active_ms) {
    app_reclaim_candidate_t c = {0};
    snprintf(c.app_id, sizeof(c.app_id), "%s", app_id);
    c.priority = priority;
    c.rss_kb = rss_kb;
    c.keep_alive = keep_alive;
    c.last_active_ms = last_active_ms;
    c.desired_running = true;
    return c;
}

static app_pressure_policy_t test_policy(void) {
    app_pressure_policy_t policy;
    edge_pressure_policy_init(&policy);
    return policy;
}

static void test_policy_defaults(void **state) {
    (void)state;
    app_pressure_policy_t policy;
    edge_pressure_policy_init(&policy);
    assert_float_equal(policy.warn_pct, 70.0, 0.001);
    assert_float_equal(policy.high_pct, 85.0, 0.001);
    assert_float_equal(policy.critical_pct, 95.0, 0.001);
    assert_int_equal(policy.step_ms, 3000);
    assert_int_equal(policy.cooldown_s, 60);
    assert_int_equal(policy.thrash_count, 3);
    assert_int_equal(policy.thrash_window_s, 3600);
    assert_int_equal(policy.idle_ms, 300000);
    assert_true(policy.reclaim_enabled);
}

static void test_monitor_only_never_suspends_or_stops(void **state) {
    (void)state;
    fake_port_t fake;
    fake_init(&fake, PRESSURE_LEVEL_CRITICAL);
    fake.candidates[0] = candidate("bg-app", 1, 1000, false, 0);
    fake.candidate_count = 1;

    app_pressure_policy_t policy = test_policy();
    policy.reclaim_enabled = false;
    app_pressure_port_t port = fake_port(&fake);
    app_pressure_t pressure;
    edge_pressure_init(&pressure, &port, &policy);

    assert_int_equal(edge_pressure_step(&pressure, 0), APP_PRESSURE_STEP_MONITORED);
    assert_int_equal(fake.read_calls, 1);
    assert_int_equal(fake.suspend_calls, 0);
    assert_int_equal(fake.stop_calls, 0);
    assert_int_equal(fake.event_count, 1);
    assert_int_equal(fake.events[0].level, PRESSURE_LEVEL_CRITICAL);

    assert_int_equal(edge_pressure_step(&pressure, 3000), APP_PRESSURE_STEP_MONITORED);
    assert_int_equal(fake.stop_calls, 0);
    assert_int_equal(fake.event_count, 1);
}

static void test_warn_clears_cache(void **state) {
    (void)state;
    fake_port_t fake;
    fake_init(&fake, PRESSURE_LEVEL_WARN);
    app_pressure_port_t port = fake_port(&fake);
    app_pressure_t pressure;
    edge_pressure_init(&pressure, &port, NULL);

    assert_int_equal(edge_pressure_step(&pressure, 0), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(fake.clear_calls, 1);
    assert_int_equal(fake.suspend_calls, 0);
    assert_int_equal(fake.stop_calls, 0);
    assert_true(fake_has_event(&fake, "", RECLAIM_REASON_BUDGET, "cleared caches"));

    assert_int_equal(edge_pressure_step(&pressure, 1500), APP_PRESSURE_STEP_IDLE);
    assert_int_equal(fake.clear_calls, 1);

    assert_int_equal(edge_pressure_step(&pressure, 3000), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(fake.clear_calls, 2);
}

static void test_high_suspends_idle_only(void **state) {
    (void)state;
    const uint64_t now = 1000000;
    fake_port_t fake;
    fake_init(&fake, PRESSURE_LEVEL_HIGH);
    fake.candidates[0] = candidate("idle-app", 5, 1000, false, now - 400000);
    fake.candidates[1] = candidate("active-app", 5, 5000, false, now - 1000);
    fake.candidates[2] = candidate("core-app", 5, 9000, true, now - 400000);
    fake.candidates[3] = candidate("asleep-app", 5, 8000, false, now - 400000);
    fake.candidates[3].suspended = true;
    fake.candidate_count = 4;

    app_pressure_port_t port = fake_port(&fake);
    app_pressure_t pressure;
    edge_pressure_init(&pressure, &port, NULL);

    assert_int_equal(edge_pressure_step(&pressure, now), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(fake.suspend_calls, 1);
    assert_string_equal(fake.suspended[0], "idle-app");
    assert_true(fake_has_event(&fake, "idle-app", RECLAIM_REASON_RECLAIMED, "suspended"));
}

static void test_critical_reclaims_in_band_order(void **state) {
    (void)state;
    fake_port_t fake;
    fake_init(&fake, PRESSURE_LEVEL_CRITICAL);
    fake.candidates[0] = candidate("bg-app", 1, 100, false, 0);
    fake.candidates[1] = candidate("normal-app", 5, 5000, false, 0);
    fake.candidates[2] = candidate("critical-app", 9, 9000, false, 0);
    fake.candidate_count = 3;

    app_pressure_port_t port = fake_port(&fake);
    app_pressure_t pressure;
    edge_pressure_init(&pressure, &port, NULL);

    assert_int_equal(edge_pressure_step(&pressure, 0), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(fake.stop_calls, 1);
    assert_string_equal(fake.stopped[0], "bg-app");
    assert_int_equal(fake.stop_reasons[0], RECLAIM_REASON_RECLAIMED);

    assert_int_equal(edge_pressure_step(&pressure, 3000), APP_PRESSURE_STEP_MONITORED);
    assert_int_equal(fake.stop_calls, 1);

    assert_int_equal(edge_pressure_step(&pressure, 60000), APP_PRESSURE_STEP_ACTED);
    assert_string_equal(fake.stopped[1], "normal-app");

    assert_int_equal(edge_pressure_step(&pressure, 120000), APP_PRESSURE_STEP_ACTED);
    assert_string_equal(fake.stopped[2], "critical-app");
    assert_int_equal(fake.stop_calls, 3);

    assert_int_equal(edge_pressure_step(&pressure, 180000), APP_PRESSURE_STEP_MONITORED);
    assert_int_equal(fake.stop_calls, 3);
}

static void test_keep_alive_is_exempt(void **state) {
    (void)state;
    const uint64_t now = 1000000;

    fake_port_t fake;
    fake_init(&fake, PRESSURE_LEVEL_CRITICAL);
    fake.candidates[0] = candidate("core-app", 10, 10000, true, now - 400000);
    fake.candidates[1] = candidate("bg-app", 2, 500, false, now - 400000);
    fake.candidate_count = 2;
    app_pressure_port_t port = fake_port(&fake);
    app_pressure_t pressure;
    edge_pressure_init(&pressure, &port, NULL);

    assert_int_equal(edge_pressure_step(&pressure, now), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(fake.stop_calls, 1);
    assert_string_equal(fake.stopped[0], "bg-app");
    assert_int_equal(edge_pressure_step(&pressure, now + 60000), APP_PRESSURE_STEP_MONITORED);
    assert_int_equal(fake.stop_calls, 1);

    fake_port_t fake_high;
    fake_init(&fake_high, PRESSURE_LEVEL_HIGH);
    fake_high.candidates[0] = candidate("core-app", 10, 10000, true, now - 400000);
    fake_high.candidates[1] = candidate("bg-app", 2, 500, false, now - 400000);
    fake_high.candidate_count = 2;
    app_pressure_port_t high_port = fake_port(&fake_high);
    app_pressure_t high_pressure;
    edge_pressure_init(&high_pressure, &high_port, NULL);

    assert_int_equal(edge_pressure_step(&high_pressure, now), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(fake_high.suspend_calls, 1);
    assert_string_equal(fake_high.suspended[0], "bg-app");
}

static void test_cooldown_between_reclaims(void **state) {
    (void)state;
    const uint64_t now = 1000000;
    fake_port_t fake;
    fake_init(&fake, PRESSURE_LEVEL_HIGH);
    fake.candidates[0] = candidate("app-a", 5, 2000, false, now - 400000);
    fake.candidates[1] = candidate("app-b", 5, 1000, false, now - 400000);
    fake.candidate_count = 2;

    app_pressure_port_t port = fake_port(&fake);
    app_pressure_t pressure;
    edge_pressure_init(&pressure, &port, NULL);

    assert_int_equal(edge_pressure_step(&pressure, now), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(fake.suspend_calls, 1);
    assert_string_equal(fake.suspended[0], "app-a");

    assert_int_equal(edge_pressure_step(&pressure, now + 3000), APP_PRESSURE_STEP_MONITORED);
    assert_int_equal(fake.suspend_calls, 1);

    assert_int_equal(edge_pressure_step(&pressure, now + 60000), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(fake.suspend_calls, 2);
    assert_string_equal(fake.suspended[1], "app-b");
}

static void test_step_interval(void **state) {
    (void)state;
    fake_port_t fake;
    fake_init(&fake, PRESSURE_LEVEL_NORMAL);
    app_pressure_port_t port = fake_port(&fake);
    app_pressure_t pressure;
    edge_pressure_init(&pressure, &port, NULL);

    assert_int_equal(edge_pressure_step(&pressure, 0), APP_PRESSURE_STEP_MONITORED);
    assert_int_equal(fake.read_calls, 1);

    assert_int_equal(edge_pressure_step(&pressure, 2000), APP_PRESSURE_STEP_IDLE);
    assert_int_equal(fake.read_calls, 1);

    assert_int_equal(edge_pressure_step(&pressure, 3000), APP_PRESSURE_STEP_MONITORED);
    assert_int_equal(fake.read_calls, 2);
}

static void test_thrash_alert_then_block(void **state) {
    (void)state;
    fake_port_t fake;
    fake_init(&fake, PRESSURE_LEVEL_CRITICAL);
    fake.remove_on_stop = false;
    fake.candidates[0] = candidate("noisy-app", 5, 1000, false, 0);
    fake.candidate_count = 1;

    app_pressure_port_t port = fake_port(&fake);
    app_pressure_t pressure;
    edge_pressure_init(&pressure, &port, NULL);

    assert_int_equal(edge_pressure_step(&pressure, 0), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(edge_pressure_step(&pressure, 60000), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(edge_pressure_step(&pressure, 120000), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(fake.stop_calls, 3);
    assert_true(fake_has_event(&fake, "noisy-app", RECLAIM_REASON_RECLAIMED, "thrash"));

    assert_int_equal(edge_pressure_step(&pressure, 180000), APP_PRESSURE_STEP_MONITORED);
    assert_int_equal(fake.stop_calls, 3);

    assert_int_equal(edge_pressure_step(&pressure, 3600000), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(fake.stop_calls, 4);
}

static void test_recovery_activates_desired_running(void **state) {
    (void)state;
    fake_port_t fake;
    fake_init(&fake, PRESSURE_LEVEL_NORMAL);
    fake.candidates[0] = candidate("run-app", 5, 1000, false, 0);
    fake.candidates[0].suspended = true;
    fake.candidates[1] = candidate("off-app", 5, 1000, false, 0);
    fake.candidates[1].suspended = true;
    fake.candidates[1].desired_running = false;
    fake.candidates[2] = candidate("live-app", 5, 1000, false, 0);
    fake.candidate_count = 3;

    app_pressure_port_t port = fake_port(&fake);
    app_pressure_t pressure;
    edge_pressure_init(&pressure, &port, NULL);

    assert_int_equal(edge_pressure_step(&pressure, 0), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(fake.activate_calls, 1);
    assert_string_equal(fake.activated[0], "run-app");
    assert_true(fake_has_event(&fake, "run-app", RECLAIM_REASON_BUDGET, "activated"));

    assert_int_equal(edge_pressure_step(&pressure, 3000), APP_PRESSURE_STEP_MONITORED);
    assert_int_equal(fake.activate_calls, 1);
}

static void test_oom_is_failure_reclaim_is_not(void **state) {
    (void)state;
    const uint64_t now = 1000000;
    fake_port_t fake;
    fake_init(&fake, PRESSURE_LEVEL_HIGH);
    fake.candidates[0] = candidate("idle-app", 5, 1000, false, now - 400000);
    fake.candidate_count = 1;

    app_pressure_port_t port = fake_port(&fake);
    app_pressure_t pressure;
    edge_pressure_init(&pressure, &port, NULL);

    assert_int_equal(edge_pressure_step(&pressure, now), APP_PRESSURE_STEP_ACTED);
    assert_int_equal(edge_pressure_failure_count(&pressure), 0);
    assert_true(fake_has_event(&fake, "idle-app", RECLAIM_REASON_RECLAIMED, "suspended"));

    edge_pressure_note_oom(&pressure, "victim-app");
    assert_int_equal(edge_pressure_failure_count(&pressure), 1);
    assert_true(fake_has_event(&fake, "victim-app", RECLAIM_REASON_OOM_KILLED, "oom"));

    edge_pressure_note_oom(&pressure, "victim-app");
    assert_int_equal(edge_pressure_failure_count(&pressure), 2);
}

static void test_psi_parsing(void **state) {
    (void)state;
    app_pressure_policy_t policy = test_policy();
    policy.warn_pct = 10.0;
    policy.high_pct = 25.0;
    policy.critical_pct = 50.0;
    pressure_level_t level = PRESSURE_LEVEL_CRITICAL;

    assert_int_equal(edge_pressure_parse_psi(
                         "some avg10=0.00 avg60=0.00 avg300=0.00 total=0\n"
                         "full avg10=0.00 avg60=0.00 avg300=0.00 total=0\n",
                         &policy, &level),
                     0);
    assert_int_equal(level, PRESSURE_LEVEL_NORMAL);

    assert_int_equal(edge_pressure_parse_psi("some avg10=12.50 avg60=0.00 total=1\n", &policy,
                                             &level),
                     0);
    assert_int_equal(level, PRESSURE_LEVEL_WARN);

    assert_int_equal(edge_pressure_parse_psi("some avg10=30.00 avg60=0.00 total=1\n", &policy,
                                             &level),
                     0);
    assert_int_equal(level, PRESSURE_LEVEL_HIGH);

    assert_int_equal(edge_pressure_parse_psi("some avg10=75.00 avg60=0.00 total=1\n", &policy,
                                             &level),
                     0);
    assert_int_equal(level, PRESSURE_LEVEL_CRITICAL);

    assert_int_equal(edge_pressure_parse_psi(
                         "full avg10=99.00 avg60=0.00 avg300=0.00 total=1\n", &policy, &level),
                     -1);
    assert_int_equal(edge_pressure_parse_psi("", &policy, &level), -1);
}

static void test_meminfo_parsing(void **state) {
    (void)state;
    app_pressure_policy_t policy = test_policy();
    policy.warn_pct = 50.0;
    policy.high_pct = 70.0;
    policy.critical_pct = 90.0;
    pressure_level_t level = PRESSURE_LEVEL_CRITICAL;

    assert_int_equal(edge_pressure_parse_meminfo(
                         "MemTotal:       1000 kB\nMemAvailable:    600 kB\n", &policy, &level),
                     0);
    assert_int_equal(level, PRESSURE_LEVEL_NORMAL);

    assert_int_equal(edge_pressure_parse_meminfo(
                         "MemTotal:       1000 kB\nMemAvailable:    400 kB\n", &policy, &level),
                     0);
    assert_int_equal(level, PRESSURE_LEVEL_WARN);

    assert_int_equal(edge_pressure_parse_meminfo(
                         "MemTotal:       1000 kB\nMemAvailable:    250 kB\n", &policy, &level),
                     0);
    assert_int_equal(level, PRESSURE_LEVEL_HIGH);

    assert_int_equal(edge_pressure_parse_meminfo(
                         "MemTotal:       1000 kB\nMemAvailable:     50 kB\n", &policy, &level),
                     0);
    assert_int_equal(level, PRESSURE_LEVEL_CRITICAL);

    assert_int_equal(edge_pressure_parse_meminfo(
                         "MemTotal:       1000 kB\nMemAvailable:   1200 kB\n", &policy, &level),
                     0);
    assert_int_equal(level, PRESSURE_LEVEL_NORMAL);

    assert_int_equal(edge_pressure_parse_meminfo(
                         "MemTotal:       1000 kB\nMemFree:         10 kB\n", &policy, &level),
                     -1);
}

static void test_no_psi_falls_back_to_meminfo(void **state) {
    (void)state;
    app_pressure_policy_t policy = test_policy();
    policy.warn_pct = 50.0;
    policy.high_pct = 70.0;
    policy.critical_pct = 90.0;
    pressure_level_t level = PRESSURE_LEVEL_NORMAL;
    const char *meminfo = "MemTotal:       1000 kB\nMemAvailable:    400 kB\n";

    assert_int_equal(edge_pressure_level_from_sources(NULL, meminfo, &policy, &level), 0);
    assert_int_equal(level, PRESSURE_LEVEL_WARN);

    assert_int_equal(edge_pressure_level_from_sources("full avg10=0.00 total=0\n", meminfo, &policy,
                                                      &level),
                     0);
    assert_int_equal(level, PRESSURE_LEVEL_WARN);

    assert_int_equal(edge_pressure_level_from_sources("some avg10=75.00 avg60=0.00 total=1\n",
                                                      meminfo, &policy, &level),
                     0);
    assert_int_equal(level, PRESSURE_LEVEL_HIGH);

    assert_int_equal(edge_pressure_level_from_sources(NULL, NULL, &policy, &level), -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_policy_defaults),
        cmocka_unit_test(test_monitor_only_never_suspends_or_stops),
        cmocka_unit_test(test_warn_clears_cache),
        cmocka_unit_test(test_high_suspends_idle_only),
        cmocka_unit_test(test_critical_reclaims_in_band_order),
        cmocka_unit_test(test_keep_alive_is_exempt),
        cmocka_unit_test(test_cooldown_between_reclaims),
        cmocka_unit_test(test_step_interval),
        cmocka_unit_test(test_thrash_alert_then_block),
        cmocka_unit_test(test_recovery_activates_desired_running),
        cmocka_unit_test(test_oom_is_failure_reclaim_is_not),
        cmocka_unit_test(test_psi_parsing),
        cmocka_unit_test(test_meminfo_parsing),
        cmocka_unit_test(test_no_psi_falls_back_to_meminfo),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
