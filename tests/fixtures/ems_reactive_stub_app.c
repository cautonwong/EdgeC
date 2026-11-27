/**
 * @file ems_reactive_stub_app.c
 * @brief Native stand-in for the P2-03b PF strategy app.
 *
 * No wasm toolchain exists in this environment, so this process speaks Host ABI
 * v1 over the inherited --fd exactly like cmd/app-host would for the installed
 * wasm package, but the policy runs natively through the pure
 * iems_pf_evaluate() module. It keeps the wasm app's lifecycle (app_init /
 * app_on_event / app_stop), reads the four read-only PCC product tags every
 * tick, publishes only its own app.ems.* points, and degrades to read-only when
 * the host denies a publish.
 */

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "edge_app_sdk.h"
#include "iems_pf_decision.h"

static edge_app_sdk_t *g_sdk;
static volatile sig_atomic_t g_stop;
static uint32_t g_p_kw;   /* product tag pcc_p_kw, read-only */
static uint32_t g_q_kvar; /* product tag pcc_q_kvar, read-only */
static uint32_t g_pf;     /* product tag pcc_pf, read-only */
static uint32_t g_online; /* product tag pcc_online, read-only */
static uint32_t g_q_target; /* app.ems.q_target_kvar, own publish point */
static uint32_t g_state;    /* app.ems.strategy_state, own publish point */
static uint32_t g_ts_ms;    /* app.ems.strategy_ts_ms, own publish point */
static int g_write_disabled;
static iems_pf_state_t g_pf_state;
static uint64_t g_tick_seq;
static iems_pf_decision_t g_last_decision;

static const char *const k_reasons[] = {
    "NONE",        "BAD_CONFIG",   "DISABLED",     "NO_OUTPUT",   "METER_OFFLINE",
    "PF_OUT_OF_RANGE", "BAD_MEASUREMENT", "LOW_POWER", "IN_BAND",   "WAITING_START",
    "RUNNING",     "ENABLED",      "FAULT_HOLD",   "FAULT_RECOVERED",
};

static const char *reason_name(iems_pf_reason_t reason) {
    size_t index = (size_t)reason;
    return index < sizeof(k_reasons) / sizeof(k_reasons[0]) ? k_reasons[index] : "UNKNOWN";
}

static void on_signal(int sig) {
    (void)sig;
    g_stop = 1;
}

static uint32_t resolve(const char *name, const char *want_access) {
    uint32_t handle = 0;
    const char *access = NULL;
    if (edge_app_point_resolve(g_sdk, name, &handle, &access) != NULL) return 0;
    if (!access || strcmp(access, want_access) != 0) return 0;
    return handle;
}

/* Host setting_get returns the validated JSON scalar (true/false/number).
 * Numeric fields go through strtod; only `enable` is a boolean. */
static int setting_value(const char *key, char *out, size_t cap) {
    return edge_app_setting_get(g_sdk, key, out, cap) == NULL ? 0 : -1;
}

static double setting_number(const char *key, double fallback) {
    char text[32];
    if (setting_value(key, text, sizeof(text)) != 0) return fallback;
    char *end = NULL;
    double value = strtod(text, &end);
    return end != text ? value : fallback;
}

static int setting_int(const char *key, int fallback) {
    return (int)setting_number(key, (double)fallback);
}

static int setting_enabled(void) {
    char text[16];
    if (setting_value("enable", text, sizeof(text)) != 0) return 0;
    return strcmp(text, "true") == 0;
}

/* host_read always yields a numeric sample; a failed read leaves the caller's
 * fallback (NAN for P/Q/PF, 0 for online) in place. */
static int read_number(uint32_t handle, double *out) {
    if (handle == 0) return -1;
    edge_app_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    if (edge_app_point_read(g_sdk, handle, &sample) != NULL) return -1;
    *out = sample.value.number;
    return 0;
}

static void app_init(void *ctx) {
    (void)ctx;
    g_p_kw = resolve("pcc_p_kw", "read");
    g_q_kvar = resolve("pcc_q_kvar", "read");
    g_pf = resolve("pcc_pf", "read");
    g_online = resolve("pcc_online", "read");
    g_q_target = resolve("app.ems.q_target_kvar", "publish");
    g_state = resolve("app.ems.strategy_state", "publish");
    g_ts_ms = resolve("app.ems.strategy_ts_ms", "publish");
    g_pf_state = iems_pf_state_init();
    g_tick_seq = 0;
    memset(&g_last_decision, 0, sizeof(g_last_decision));
    g_last_decision.state = IEMS_PF_STATE_DISABLED;
    g_last_decision.reason = IEMS_PF_REASON_DISABLED;
    edge_app_log(g_sdk, "info", "ems.pf: control ready");
}

static int publish_or_degrade(uint32_t handle, const edge_app_sample_t *sample) {
    const char *err = edge_app_point_publish(g_sdk, handle, sample);
    if (err == NULL) return 0;
    g_write_disabled = 1;
    char message[96];
    snprintf(message, sizeof(message), "ems.pf: publish %s, read-only", err);
    edge_app_log(g_sdk, "warn", message);
    return -1;
}

static int publish_number(uint32_t handle, double value) {
    edge_app_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    sample.value.kind = EDGE_APP_VALUE_FLOAT;
    sample.value.number = value;
    sample.quality = "good";
    return publish_or_degrade(handle, &sample);
}

static int publish_integer(uint32_t handle, int64_t value) {
    edge_app_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    sample.value.kind = EDGE_APP_VALUE_INTEGER;
    sample.value.integer = value;
    sample.quality = "good";
    return publish_or_degrade(handle, &sample);
}

static int app_on_event(void *ctx, const char *event_json) {
    (void)ctx;
    if (event_json == NULL || strstr(event_json, "\"tick\"") == NULL) return 0;

    /* Every tick reads the read-only inputs; enable and the read-only degrade
     * gate only the publishes. control_cycle_s is the decision period in
     * ticks, so skipped ticks log the previous decision and publish nothing. */
    double p_kw = NAN;
    double q_kvar = NAN;
    double pf = NAN;
    double online = 0.0;
    (void)read_number(g_p_kw, &p_kw);
    (void)read_number(g_q_kvar, &q_kvar);
    (void)read_number(g_pf, &pf);
    (void)read_number(g_online, &online);

    int enabled = setting_enabled();
    iems_pf_cfg_t cfg = {0};
    cfg.en_pf_ctrl = setting_int("en_pf_ctrl", 0);
    cfg.reactive_mode = setting_int("reactive_mode", 0);
    cfg.pf_target = setting_number("pf_target", IEMS_PF_DEFAULT_PF_TARGET);
    cfg.pf_deadband = setting_number("pf_deadband", IEMS_PF_DEFAULT_DEADBAND);
    cfg.p_min_kw = setting_number("p_min_kw", IEMS_PF_DEFAULT_P_MIN_KW);
    cfg.start_delay_s = setting_int("start_delay_s", IEMS_PF_DEFAULT_START_DELAY_S);
    cfg.hold_s = setting_int("hold_s", IEMS_PF_DEFAULT_HOLD_S);
    cfg.control_cycle_s = setting_int("control_cycle_s", IEMS_PF_DEFAULT_CONTROL_CYCLE_S);
    if (!enabled) cfg.en_pf_ctrl = 0;

    uint64_t now_ms = 0;
    (void)edge_app_now(g_sdk, &now_ms);
    iems_pf_input_t in = {
        .now_ms = now_ms,
        .p_kw = p_kw,
        .q_kvar = q_kvar,
        .pf = pf,
        .meter_online = online != 0.0,
    };
    g_tick_seq++;
    int cycle = cfg.control_cycle_s > 0 ? cfg.control_cycle_s : 1;
    int evaluated = !enabled || cycle <= 1 || (g_tick_seq % (uint64_t)cycle) == 0;
    iems_pf_decision_t decision = g_last_decision;
    if (evaluated) {
        decision = iems_pf_evaluate(&cfg, &g_pf_state, &in);
        g_pf_state = decision.next_state;
        g_last_decision = decision;
    }

    /* Observable trace: P/Q/PF, meter state, decision state, Q target, reason. */
    char message[192];
    snprintf(message, sizeof(message),
             "ems.pf: p=%.3f q=%.3f pf=%.3f online=%d state=%d q_target=%.3f reason=%s", p_kw,
             q_kvar, pf, in.meter_online ? 1 : 0, (int)decision.state, decision.q_target_kvar,
             reason_name(decision.reason));
    edge_app_log(g_sdk, "info", message);

    if (!evaluated) return 0;
    if (!enabled || g_write_disabled || g_q_target == 0 || g_state == 0 || g_ts_ms == 0) return 0;
    if (publish_number(g_q_target, decision.q_target_kvar) != 0) return 0;
    if (publish_integer(g_state, (int64_t)decision.state) != 0) return 0;
    (void)publish_integer(g_ts_ms, (int64_t)now_ms);
    return 0;
}

int main(int argc, char **argv) {
    int fd = -1;
    int close_fd = -1;
    for (int i = 1; i + 1 < argc; i += 2) {
        if (strcmp(argv[i], "--fd") == 0) fd = atoi(argv[i + 1]);
        if (strcmp(argv[i], "--close-fd") == 0) close_fd = atoi(argv[i + 1]);
    }
    if (fd < 0) {
        fprintf(stderr, "ems-pf-app: missing --fd\n");
        return 2;
    }
    if (close_fd >= 0 && close_fd != fd) close(close_fd);

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    g_sdk = edge_app_sdk_create(fd);
    if (!g_sdk) return 3;
    edge_app_sdk_set_exports(g_sdk, NULL, app_on_event, NULL, NULL);
    app_init(NULL);

    char frame[4096];
    char response[256];
    while (!g_stop && edge_app_sdk_next_frame(g_sdk, frame, sizeof(frame)) == 0) {
        if (edge_app_sdk_dispatch(g_sdk, frame, response, sizeof(response)) != 0) break;
        if (edge_app_sdk_send_frame(g_sdk, response) != 0) break;
    }
    edge_app_sdk_destroy(g_sdk);
    return 0;
}
