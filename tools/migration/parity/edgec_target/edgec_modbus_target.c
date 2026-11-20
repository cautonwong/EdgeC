/*
 * P1-01 parity target: drive the real EdgeC Modbus RTU codec.
 *
 * Real EdgeC code (libedge_codec.a): request encode, request decode, reply
 * decode and CRC16. Simulated: the meter only. It decodes each request with
 * the real codec, answers from a seeded holding register and frames the reply.
 *
 * Holding register map (0-based; 40001 + addr on the wire), all int16 unless noted:
 *   addr 0  pcs1.active_power        FC03 read   deci-kW      (raw / 10)
 *   addr 1  pcs1.reactive_power      FC03 read   deci-kvar
 *   addr 2  pcs1.set_active_power    FC06 write  deci-kW target_kw
 *   addr 3  pcs1.set_reactive_power  FC06 write  deci-kvar target_kvar
 *   addr 4  bms1.soc                 FC03 read   deci-%       (raw / 10)
 *   addr 5  pcs1.comm_fault          FC03 read   1 raise, 0 clear
 *   addr 6  meter1.grid_power        FC03 read   deci-kW
 */
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "modbus_codec.h"
#include "net_buf.h"

#define PARITY_SLAVE 1
#define PARITY_SCALE 10.0 /* one register carries engineering value * 10 */

enum key_kind { KIND_POINT, KIND_COMMAND, KIND_ALARM };

typedef struct {
    const char *key;
    enum key_kind kind;
    uint16_t addr;
    const char *num_field;  /* command: object field holding the target */
    const char *text_field; /* command: req_id; alarm: state */
    const char *code_field; /* alarm: code */
} key_map_t;

static const key_map_t KEY_MAP[] = {
    {"pcs1.active_power", KIND_POINT, 0, NULL, NULL, NULL},
    {"pcs1.reactive_power", KIND_POINT, 1, NULL, NULL, NULL},
    {"pcs1.set_active_power", KIND_COMMAND, 2, "target_kw", "req_id", NULL},
    {"pcs1.set_reactive_power", KIND_COMMAND, 3, "target_kvar", "req_id", NULL},
    {"bms1.soc", KIND_POINT, 4, NULL, NULL, NULL},
    {"pcs1.comm_fault", KIND_ALARM, 5, NULL, "state", "code"},
    {"meter1.grid_power", KIND_POINT, 6, NULL, NULL, NULL},
};
#define KEY_MAP_N ((int)(sizeof(KEY_MAP) / sizeof(KEY_MAP[0])))

typedef struct {
    long long t_ms;
    const key_map_t *map;
    int value_is_float;
    double number;
    char text[64];
    char code[64];
} event_t;

static int g_line;

static int fail(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "edgec-modbus-target: line %d: ", g_line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    return -1;
}

/* --- minimal JSON scanning (v1 traces only: flat objects, no \u escapes) --- */

static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
}

static const char *skip_value(const char *p) {
    p = skip_ws(p);
    if (*p == '"') {
        for (p++; *p; p++) {
            if (*p == '\\' && p[1]) {
                p++;
                continue;
            }
            if (*p == '"') return p + 1;
        }
        return p;
    }
    if (*p == '{' || *p == '[') {
        char open = *p, close = (open == '{') ? '}' : ']';
        int depth = 0;
        while (*p) {
            if (*p == '"') {
                p = skip_value(p);
                continue;
            }
            if (*p == open) depth++;
            else if (*p == close && --depth == 0) return p + 1;
            p++;
        }
        return p;
    }
    while (*p && *p != ',' && *p != '}' && *p != ']') p++;
    return p;
}

/* Pointer to a top-level field's value, or NULL. */
static const char *json_field(const char *obj, const char *name) {
    obj = skip_ws(obj);
    if (*obj != '{') return NULL;
    const char *p = obj + 1;
    while (1) {
        p = skip_ws(p);
        if (*p == '}') return NULL;
        if (*p != '"') return NULL;
        char key[64];
        size_t i = 0;
        for (p++; *p && *p != '"' && i + 1 < sizeof key; p++) key[i++] = *p;
        key[i] = '\0';
        if (*p != '"') return NULL;
        p = skip_ws(p + 1);
        if (*p != ':') return NULL;
        p = skip_ws(p + 1);
        if (strcmp(key, name) == 0) return p;
        p = skip_ws(skip_value(p));
        if (*p != ',') return NULL;
        p++;
    }
}

static int json_string(const char *p, char *dst, size_t n) {
    p = skip_ws(p);
    if (*p != '"') return -1;
    size_t i = 0;
    for (p++; *p && *p != '"'; p++) {
        char c = *p;
        if (c == '\\' && p[1]) {
            p++;
            switch (*p) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '"': case '\\': case '/': c = *p; break;
                default: c = *p; break; /* \u is out of scope for v1 traces */
            }
        }
        if (i + 1 < n) dst[i++] = c;
    }
    if (*p != '"') return -1;
    dst[i] = '\0';
    return 0;
}

static int json_number(const char *p, double *value, int *is_float) {
    p = skip_ws(p);
    char *end = NULL;
    errno = 0;
    double v = strtod(p, &end);
    if (end == p || errno != 0) return -1;
    *is_float = 0;
    for (const char *q = p; q < end; q++) {
        if (*q == '.' || *q == 'e' || *q == 'E') {
            *is_float = 1;
            break;
        }
    }
    *value = v;
    return 0;
}

static const key_map_t *lookup_key(const char *key) {
    for (int i = 0; i < KEY_MAP_N; i++)
        if (strcmp(KEY_MAP[i].key, key) == 0) return &KEY_MAP[i];
    return NULL;
}

static int parse_event(const char *line, event_t *ev) {
    const char *f;
    double v;
    int is_float;
    char kind[16], key[64];

    f = json_field(line, "v");
    if (!f || json_number(f, &v, &is_float) != 0 || is_float || v != 1.0)
        return fail("v must be the integer 1");
    f = json_field(line, "t_ms");
    if (!f || json_number(f, &v, &is_float) != 0 || is_float || v < 0.0)
        return fail("t_ms must be a non-negative integer");
    ev->t_ms = (long long)v;
    if (!json_field(line, "source") ||
        json_string(json_field(line, "source"), kind, sizeof kind) != 0 || !kind[0])
        return fail("source must be a non-empty string");
    if (!json_field(line, "kind") || json_string(json_field(line, "kind"), kind, sizeof kind) != 0)
        return fail("kind is missing");
    if (!json_field(line, "key") || json_string(json_field(line, "key"), key, sizeof key) != 0)
        return fail("key is missing");
    ev->map = lookup_key(key);
    if (!ev->map) return fail("no register mapping for key %s", key);

    const char *value = json_field(line, "value");
    if (!value) return fail("value is missing");

    ev->text[0] = '\0';
    ev->code[0] = '\0';
    if (strcmp(kind, "point") == 0) {
        if (ev->map->kind != KIND_POINT) return fail("key %s is not a point", key);
        if (json_number(value, &ev->number, &ev->value_is_float) != 0)
            return fail("point value must be a number (null is not representable)");
    } else if (strcmp(kind, "command") == 0) {
        if (ev->map->kind != KIND_COMMAND) return fail("key %s is not a command", key);
        if (!json_field(value, ev->map->num_field) ||
            json_number(json_field(value, ev->map->num_field), &ev->number,
                        &ev->value_is_float) != 0)
            return fail("command needs a numeric %s", ev->map->num_field);
        if (!json_field(value, ev->map->text_field) ||
            json_string(json_field(value, ev->map->text_field), ev->text, sizeof ev->text) != 0)
            return fail("command needs a string %s", ev->map->text_field);
    } else if (strcmp(kind, "alarm") == 0) {
        if (ev->map->kind != KIND_ALARM) return fail("key %s is not an alarm", key);
        if (!json_field(value, "state") ||
            json_string(json_field(value, "state"), ev->text, sizeof ev->text) != 0)
            return fail("alarm needs a string state");
        if (strcmp(ev->text, "raise") != 0 && strcmp(ev->text, "clear") != 0)
            return fail("alarm state must be raise or clear");
        if (!json_field(value, "code") ||
            json_string(json_field(value, "code"), ev->code, sizeof ev->code) != 0)
            return fail("alarm needs a string code");
    } else {
        return fail("kind must be point, command or alarm");
    }
    return 0;
}

/* --- frame plumbing --- */

static int reg_from_value(double v, uint16_t *raw) {
    double scaled = v * PARITY_SCALE;
    if (scaled < -32768.0 || scaled > 32767.0)
        return fail("value %g does not fit one int16 register at x10 scale", v);
    *raw = (uint16_t)(int16_t)llround(scaled);
    return 0;
}

static double value_from_reg(uint16_t raw) { return (double)(int16_t)raw / PARITY_SCALE; }

static int decode_req(const net_buf_t *frame, uint8_t *slave, uint8_t *fc, uint8_t **data,
                      size_t *data_len) {
    struct net_buf_simple in;
    net_buf_simple_init_with_data(&in, frame->b.data, frame->b.len);
    in.len = frame->b.len;
    int rc = modbus_rtu_decode_req_frame_ex(&in, false, slave, fc, data, data_len);
    if (rc != 0) return fail("real codec rejected the encoded request (%d)", rc);
    return 0;
}

static int decode_reply(uint8_t *frame, size_t len, uint8_t *slave, uint8_t *fc, uint8_t **data,
                        size_t *data_len) {
    struct net_buf_simple in;
    net_buf_simple_init_with_data(&in, frame, len);
    in.len = (uint16_t)len;
    int rc = modbus_rtu_decode_frame_ex(&in, false, slave, fc, data, data_len);
    if (rc != 0) return fail("real codec rejected the meter reply (%d)", rc);
    return 0;
}

static size_t seal_reply(uint8_t *frame, size_t body_len) {
    uint16_t crc = modbus_calculate_crc(frame, body_len);
    frame[body_len] = (uint8_t)(crc & 0xFF);
    frame[body_len + 1] = (uint8_t)(crc >> 8);
    return body_len + 2;
}

/* Encode FC03, let the simulated meter answer with seed, decode the reply. */
static int read_register(uint16_t addr, uint16_t seed, uint16_t *value) {
    uint8_t req_mem[16];
    net_buf_t req = {0};
    req.b.__buf = req_mem;
    req.b.size = (uint16_t)sizeof req_mem;
    net_buf_simple_init(&req.b, 0);
    if (modbus_rtu_encode_read_ex(PARITY_SLAVE, MODBUS_FC_READ_HOLDING_REGS, addr, 1, false,
                                  &req) != 0)
        return fail("modbus_rtu_encode_read_ex failed for register %u", addr);

    uint8_t slave, fc;
    uint8_t *data;
    size_t data_len;
    if (decode_req(&req, &slave, &fc, &data, &data_len) != 0) return -1;
    if (fc != MODBUS_FC_READ_HOLDING_REGS || data_len < 4)
        return fail("meter decoded an unexpected request");
    uint16_t req_addr = (uint16_t)((data[0] << 8) | data[1]);
    uint16_t req_qty = (uint16_t)((data[2] << 8) | data[3]);
    if (req_addr != addr || req_qty != 1)
        return fail("meter saw address %u quantity %u, expected %u/1", req_addr, req_qty, addr);

    uint8_t reply[16];
    reply[0] = slave;
    reply[1] = fc;
    reply[2] = 2;
    reply[3] = (uint8_t)(seed >> 8);
    reply[4] = (uint8_t)(seed & 0xFF);
    size_t len = seal_reply(reply, 5);

    uint8_t rslave, rfc;
    uint8_t *rdata;
    size_t rlen;
    if (decode_reply(reply, len, &rslave, &rfc, &rdata, &rlen) != 0) return -1;
    if (rslave != PARITY_SLAVE || rfc != MODBUS_FC_READ_HOLDING_REGS || rlen < 3 ||
        rdata[0] != 2)
        return fail("master decoded an unexpected reply");
    *value = (uint16_t)((rdata[1] << 8) | rdata[2]);
    return 0;
}

/* Encode FC06, let the simulated meter echo it, decode the echo. */
static int write_register(uint16_t addr, uint16_t raw, uint16_t *echoed) {
    uint8_t req_mem[16];
    net_buf_t req = {0};
    req.b.__buf = req_mem;
    req.b.size = (uint16_t)sizeof req_mem;
    net_buf_simple_init(&req.b, 0);
    if (modbus_rtu_encode_write_single_ex(PARITY_SLAVE, addr, raw, false, &req) != 0)
        return fail("modbus_rtu_encode_write_single_ex failed for register %u", addr);

    uint8_t slave, fc;
    uint8_t *data;
    size_t data_len;
    if (decode_req(&req, &slave, &fc, &data, &data_len) != 0) return -1;
    if (fc != MODBUS_FC_WRITE_SINGLE_REG || data_len < 4)
        return fail("meter decoded an unexpected write request");
    uint16_t req_addr = (uint16_t)((data[0] << 8) | data[1]);
    uint16_t req_raw = (uint16_t)((data[2] << 8) | data[3]);
    if (req_addr != addr || req_raw != raw)
        return fail("meter saw a different write than the master encoded");

    uint8_t reply[16];
    reply[0] = slave;
    reply[1] = fc;
    memcpy(&reply[2], data, 4); /* write reply is the request echo */
    size_t len = seal_reply(reply, 6);

    uint8_t rslave, rfc;
    uint8_t *rdata;
    size_t rlen;
    if (decode_reply(reply, len, &rslave, &rfc, &rdata, &rlen) != 0) return -1;
    if (rslave != PARITY_SLAVE || rfc != MODBUS_FC_WRITE_SINGLE_REG || rlen < 4)
        return fail("master decoded an unexpected write reply");
    if (((rdata[0] << 8) | rdata[1]) != addr || ((rdata[2] << 8) | rdata[3]) != raw)
        return fail("write reply does not echo the request");
    *echoed = (uint16_t)((rdata[2] << 8) | rdata[3]);
    return 0;
}

static void format_double(char *out, size_t n, double v, int is_float) {
    if (!is_float) {
        snprintf(out, n, "%lld", (long long)llround(v));
        return;
    }
    snprintf(out, n, "%.15g", v);
    if (!strpbrk(out, ".eE")) strncat(out, ".0", n - strlen(out) - 1);
}

static void print_point(const event_t *ev, double value) {
    char num[64];
    int as_float = ev->value_is_float || value != rint(value);
    format_double(num, sizeof num, value, as_float);
    printf("{\"v\":1,\"t_ms\":%lld,\"source\":\"edgec-modbus\",\"kind\":\"point\","
           "\"key\":\"%s\",\"value\":%s}\n",
           ev->t_ms, ev->map->key, num);
}

static int process_point(const event_t *ev, int corrupt) {
    uint16_t raw = 0;
    if (reg_from_value(ev->number, &raw) != 0) return -1;
    if (corrupt) raw++;
    uint16_t decoded = 0;
    if (read_register(ev->map->addr, raw, &decoded) != 0) return -1;
    if (decoded != raw) return fail("read-back of register %u changed the value", ev->map->addr);
    print_point(ev, value_from_reg(decoded));
    return 0;
}

static int process_command(const event_t *ev, int corrupt) {
    uint16_t raw = 0;
    if (reg_from_value(ev->number, &raw) != 0) return -1;
    if (corrupt) raw++;
    uint16_t echoed = 0;
    if (write_register(ev->map->addr, raw, &echoed) != 0) return -1;
    char num[64];
    int as_float = ev->value_is_float || value_from_reg(echoed) != rint(value_from_reg(echoed));
    format_double(num, sizeof num, value_from_reg(echoed), as_float);
    printf("{\"v\":1,\"t_ms\":%lld,\"source\":\"edgec-modbus\",\"kind\":\"command\","
           "\"key\":\"%s\",\"value\":{\"%s\":%s,\"req_id\":\"%s\"}}\n",
           ev->t_ms, ev->map->key, ev->map->num_field, num, ev->text);
    return 0;
}

static int process_alarm(const event_t *ev, int corrupt) {
    uint16_t raw = strcmp(ev->text, "raise") == 0 ? 1 : 0;
    if (corrupt) raw++;
    uint16_t decoded = 0;
    if (read_register(ev->map->addr, raw, &decoded) != 0) return -1;
    printf("{\"v\":1,\"t_ms\":%lld,\"source\":\"edgec-modbus\",\"kind\":\"alarm\","
           "\"key\":\"%s\",\"value\":{\"state\":\"%s\",\"code\":\"%s\"}}\n",
           ev->t_ms, ev->map->key, decoded ? "raise" : "clear", ev->code);
    return 0;
}

static int process_line(const char *line, const char *corrupt_key) {
    event_t ev;
    memset(&ev, 0, sizeof ev);
    if (parse_event(line, &ev) != 0) return -1;
    int corrupt = corrupt_key && strcmp(corrupt_key, ev.map->key) == 0;
    switch (ev.map->kind) {
        case KIND_POINT: return process_point(&ev, corrupt);
        case KIND_COMMAND: return process_command(&ev, corrupt);
        case KIND_ALARM: return process_alarm(&ev, corrupt);
    }
    return fail("unreachable");
}

static void usage(FILE *out) {
    fprintf(out,
            "usage: edgec_modbus_target [--input TRACE.jsonl] [--corrupt KEY]\n"
            "  reads JSONL trace events (stdin by default) and prints one trace record per"
            " event\n"
            "  --corrupt KEY  add +1 to that key's register value (parity selftest)\n");
}

int main(int argc, char **argv) {
    const char *input_path = NULL;
    const char *corrupt_key = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            input_path = argv[++i];
        } else if (strcmp(argv[i], "--corrupt") == 0 && i + 1 < argc) {
            corrupt_key = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(stdout);
            return 0;
        } else {
            usage(stderr);
            return 2;
        }
    }
    if (corrupt_key && !lookup_key(corrupt_key)) {
        fprintf(stderr, "edgec-modbus-target: --corrupt key %s is not in the register map\n",
                corrupt_key);
        return 2;
    }

    FILE *in = stdin;
    if (input_path) {
        in = fopen(input_path, "r");
        if (!in) {
            fprintf(stderr, "edgec-modbus-target: cannot open %s: %s\n", input_path,
                    strerror(errno));
            return 2;
        }
    }

    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    int rc = 0;
    while ((len = getline(&line, &cap, in)) != -1) {
        g_line++;
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        const char *p = skip_ws(line);
        if (!*p) continue;
        if (process_line(p, corrupt_key) != 0) {
            rc = 2;
            break;
        }
        fflush(stdout);
    }
    free(line);
    if (rc == 0 && ferror(in)) {
        fprintf(stderr, "edgec-modbus-target: read error after line %d\n", g_line);
        rc = 2;
    }
    if (in != stdin) fclose(in);
    return rc;
}
