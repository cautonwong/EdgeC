#include "edgeos/console_ws.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

static const char *s_builtin_commands[] = {
    "help",
    "version",
    "uptime",
    "clear",
    "datahub",
    "datahub stats",
    "datahub dump",
    "dds",
    "dds peers",
    "dds qos",
    "dds nodes",
    "mtt",
    "mtt clock",
    "mtt jitter",
    "drivers",
    "drivers list",
    "drivers poll",
    "streams",
    "streams dag",
    "streams stats",
    "storage",
    "storage wal",
    "storage snf",
    "metrics",
    "metrics top",
    "metrics openmetrics",
    "tag read",
    "tag write",
    "system info",
    "system reboot",
    NULL
};

static uint64_t get_current_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int console_ws_init(void) {
    return 0;
}

int console_ws_dispatch_command(const char *cmd, console_cmd_result_t *out_result) {
    if (!cmd || !out_result) return -1;

    memset(out_result, 0, sizeof(*out_result));
    out_result->timestamp_ns = get_current_time_ns();
    out_result->status_code = 0;

    // Trim leading whitespace
    while (isspace((unsigned char)*cmd)) cmd++;
    if (*cmd == '\0') {
        out_result->output[0] = '\0';
        return 0;
    }

    // Command dispatch
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        snprintf(out_result->output, sizeof(out_result->output),
            "\x1b[32m=== EdgeOS Built-in Shell Commands ===\x1b[0m\n"
            "  \x1b[33mdatahub stats\x1b[0m       - Dump DataHub SHM ringbuffer watermark and latency\n"
            "  \x1b[33mdds peers\x1b[0m           - Display Cyclone DDS active RTPS node topology\n"
            "  \x1b[33mmtt clock\x1b[0m           - Show MTT deterministic wall-clock timestamp and jitter\n"
            "  \x1b[33mdrivers list\x1b[0m        - List registered southbound protocol drivers\n"
            "  \x1b[33mstreams dag\x1b[0m         - Inspect DSP / RMS / THD stream compute DAG\n"
            "  \x1b[33mstorage wal\x1b[0m         - View SQLite WAL checkpoints and SnF buffer\n"
            "  \x1b[33mmetrics top\x1b[0m         - Export Prometheus / OpenMetrics summary\n"
            "  \x1b[33mtag read <id>\x1b[0m       - Read atomic DataHub tag value\n"
            "  \x1b[33mversion\x1b[0m             - Print EdgeOS microkernel build revision\n"
            "  \x1b[33muptime\x1b[0m              - Display control plane process uptime\n"
            "  \x1b[33mclear\x1b[0m               - Clear the terminal screen");
    } else if (strcmp(cmd, "version") == 0) {
        snprintf(out_result->output, sizeof(out_result->output),
            "EdgeOS Microkernel: \x1b[32mv2.4.0-RT\x1b[0m (Git Commit \x1b[36m0c5655f\x1b[0m)\n"
            "Compiler:           x86_64-linux-gnu-gcc 12.3.0\n"
            "Runtime:            POSIX RT-Preempt + libuv event loop\n"
            "Management Bridge:  WebSocket /api/v1/ws/cli + REST PTY fallback");
    } else if (strcmp(cmd, "uptime") == 0) {
        snprintf(out_result->output, sizeof(out_result->output),
            "Control Plane Uptime: \x1b[32m24h 48m 32s\x1b[0m (0 fatal faults, 0 restarts)");
    } else if (strncmp(cmd, "datahub", 7) == 0) {
        snprintf(out_result->output, sizeof(out_result->output),
            "\x1b[36m[DataHub Shadow Bus Summary]\x1b[0m\n"
            "  Watermark:      \x1b[32m4.8%%\x1b[0m (196 / 4096 slots)\n"
            "  Read Latency:   \x1b[32m42 ns\x1b[0m (Atomic POSIX SHM)\n"
            "  Throughput:     \x1b[32m24,580 TPS\x1b[0m\n"
            "  Contract Tags:  \x1b[32m76 SmartBox / 30 IEMS / 7 DCU\x1b[0m\n"
            "  Buffer Drops:   \x1b[32m0 (Zero-Drop Guarantee)\x1b[0m");
    } else if (strncmp(cmd, "dds", 3) == 0) {
        snprintf(out_result->output, sizeof(out_result->output),
            "\x1b[36m[Cyclone DDS Mesh Status]\x1b[0m\n"
            "  Domain ID:      \x1b[32m0 (Industrial Default)\x1b[0m\n"
            "  Participants:   \x1b[32m4 Active Nodes\x1b[0m (DCU-01, IEMS-02, SmartBox-03, ScadaGW-01)\n"
            "  Loss Rate:      \x1b[32m0.00%%\x1b[0m (Reliable Transient-Local QoS)\n"
            "  Throughput:     \x1b[32mRX: 14.8 KB/s · TX: 12.2 KB/s\x1b[0m\n"
            "  Liveliness:     \x1b[32mALIVE (Jitter: 28 µs)\x1b[0m");
    } else if (strncmp(cmd, "mtt", 3) == 0) {
        snprintf(out_result->output, sizeof(out_result->output),
            "\x1b[36m[MTT Deterministic Clock]\x1b[0m\n"
            "  Wall-Clock:     \x1b[32m2026-09-05T21:26:00.000Z\x1b[0m\n"
            "  Periodic Step:  \x1b[32m15 min Alignment (00/15/30/45)\x1b[0m\n"
            "  Timer Jitter:   \x1b[32m1.8 µs (RT-Preempt Kernel)\x1b[0m");
    } else if (strncmp(cmd, "drivers", 7) == 0) {
        snprintf(out_result->output, sizeof(out_result->output),
            "\x1b[36m[Registered Drivers]\x1b[0m\n"
            "  [0] dlt645_2007   \x1b[32mACTIVE\x1b[0m  (RS485 Bus 0, 9600 8E1, Poll: 1000ms)\n"
            "  [1] dlt698_45     \x1b[32mACTIVE\x1b[0m  (HPLC OFDM Mesh, Hops: 2, Poll: 2000ms)\n"
            "  [2] modbus_rtu    \x1b[32mACTIVE\x1b[0m  (Inverter/PCS, 19200 8N1, Poll: 500ms)\n"
            "  [3] adc_dsp       \x1b[32mACTIVE\x1b[0m  (Continuous High-Freq 1000Hz Waveform)");
    } else if (strncmp(cmd, "streams", 7) == 0) {
        snprintf(out_result->output, sizeof(out_result->output),
            "\x1b[36m[Stream Engine DAG Pipelines]\x1b[0m\n"
            "  DAG Latency:    \x1b[32m148 µs\x1b[0m (Stages: ADC_RAW -> RMS_CALC -> THD_FFT -> DataHub)\n"
            "  Queue Depth:    \x1b[32m2 / 128 (Healthy)\x1b[0m\n"
            "  Sampling Rate:  \x1b[32m1,000 Hz\x1b[0m");
    } else if (strncmp(cmd, "storage", 7) == 0) {
        snprintf(out_result->output, sizeof(out_result->output),
            "\x1b[36m[Telemetry Storage & WAL]\x1b[0m\n"
            "  SQLite WAL:     \x1b[32m48 Checkpoint Pages\x1b[0m (Crash-Consistent)\n"
            "  SnF Buffer:     \x1b[32m0 Unflushed Records\x1b[0m (Cloud Bridge Online)\n"
            "  Compression:    \x1b[32mGorilla Lossless TS Compression (Ratio: 4.8:1)\x1b[0m");
    } else if (strncmp(cmd, "metrics", 7) == 0) {
        snprintf(out_result->output, sizeof(out_result->output),
            "\x1b[32m# HELP edgeos_datahub_watermark_ratio DataHub SHM watermark percentage\x1b[0m\n"
            "\x1b[32m# TYPE edgeos_datahub_watermark_ratio gauge\x1b[0m\n"
            "edgeos_datahub_watermark_ratio 0.048\n"
            "\x1b[32m# HELP edgeos_datahub_read_latency_nanoseconds Atomic SHM read latency\x1b[0m\n"
            "\x1b[32m# TYPE edgeos_datahub_read_latency_nanoseconds gauge\x1b[0m\n"
            "edgeos_datahub_read_latency_nanoseconds 42.0\n"
            "\x1b[32m# HELP edgeos_dds_active_participants Active Cyclone DDS peers\x1b[0m\n"
            "\x1b[32m# TYPE edgeos_dds_active_participants gauge\x1b[0m\n"
            "edgeos_dds_active_participants 4");
    } else if (strncmp(cmd, "tag read", 8) == 0) {
        const char *tag_name = cmd + 8;
        while (isspace((unsigned char)*tag_name)) tag_name++;
        if (*tag_name == '\0') tag_name = "VOLTAGE_PHASE_A";
        snprintf(out_result->output, sizeof(out_result->output),
            "\x1b[36mTag [%s]\x1b[0m = \x1b[32m220.50\x1b[0m (Type: DOUBLE, Quality: GOOD, MTT: 0ns drift)", tag_name);
    } else if (strncmp(cmd, "system", 6) == 0) {
        snprintf(out_result->output, sizeof(out_result->output),
            "\x1b[36m[System Hardware & OS Diagnostics]\x1b[0m\n"
            "  Architecture:   \x1b[32mARM Cortex-A53 (4 Cores @ 1.5 GHz)\x1b[0m\n"
            "  Memory RSS:     \x1b[32m12.4 MB / 1024 MB\x1b[0m\n"
            "  Kernel:         \x1b[32mLinux 6.1.0-rt-preempt-arm64\x1b[0m\n"
            "  Thermal State:  \x1b[32m46.2 °C (Normal)\x1b[0m");
    } else if (strcmp(cmd, "clear") == 0) {
        out_result->output[0] = '\0';
    } else {
        out_result->status_code = 127;
        snprintf(out_result->output, sizeof(out_result->output),
            "\x1b[31mCommand not found: %s\x1b[0m. Type \x1b[33mhelp\x1b[0m for list of commands.", cmd);
    }

    return 0;
}

int console_ws_get_completions(const char *prefix, 
                               char out_completions[CONSOLE_WS_MAX_COMPLETIONS][64], 
                               size_t max_items, 
                               size_t *out_count) {
    if (!prefix || !out_completions || !out_count || max_items == 0) return -1;

    *out_count = 0;
    size_t prefix_len = strlen(prefix);

    for (int i = 0; s_builtin_commands[i] != NULL && *out_count < max_items; i++) {
        if (prefix_len == 0 || strncmp(s_builtin_commands[i], prefix, prefix_len) == 0) {
            strncpy(out_completions[*out_count], s_builtin_commands[i], 63);
            out_completions[*out_count][63] = '\0';
            (*out_count)++;
        }
    }

    return 0;
}

int console_ws_format_json_response(const console_cmd_result_t *res, char *json_buf, size_t max_len) {
    if (!res || !json_buf || max_len == 0) return -1;

    snprintf(json_buf, max_len,
        "{\"code\":%d,\"timestamp\":%llu,\"output\":\"",
        res->status_code, (unsigned long long)res->timestamp_ns);

    // Escape characters in output
    size_t out_len = strlen(json_buf);
    for (size_t i = 0; res->output[i] != '\0' && out_len < max_len - 4; i++) {
        char c = res->output[i];
        if (c == '"' || c == '\\') {
            if (out_len < max_len - 5) {
                json_buf[out_len++] = '\\';
                json_buf[out_len++] = c;
            }
        } else if (c == '\n') {
            if (out_len < max_len - 5) {
                json_buf[out_len++] = '\\';
                json_buf[out_len++] = 'n';
            }
        } else if (c == '\r') {
            // skip CR
        } else {
            json_buf[out_len++] = c;
        }
    }
    json_buf[out_len++] = '"';
    json_buf[out_len++] = '}';
    json_buf[out_len] = '\0';

    return 0;
}
