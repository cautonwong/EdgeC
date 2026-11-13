/**
 * @file edge_wasm_imports.c
 * @brief Import-section whitelist check, standard library only.
 */

#include "edge_wasm_imports.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define WASM_HEADER_SIZE 8
#define MAX_IMPORTS 1024
#define MAX_IMPORT_NAME 128

typedef struct {
    const uint8_t *data;
    size_t len;
    size_t pos;
} reader_t;

/* Kept in sync with native_symbols in modules/edge_stream/src/wasm_runtime/wasm_mgr.c. */
static const struct {
    const char *module;
    const char *name;
} k_allowed[] = {
    {"env", "host_get_tag"},
    {"env", "host_set_tag"},
    {"env", "ext_log"},
    {"env", "host_tensor_alloc"},
    {"env", "host_tensor_free"},
    /* app-host ABI, registered by cmd/app-host/main.c */
    {"env", "point_resolve"},
    {"env", "point_read"},
    {"env", "point_publish"},
    {"env", "app_log"},
    {"env", "app_now"},
};

static int read_u8(reader_t *r, uint8_t *out) {
    if (r->pos >= r->len) return -1;
    *out = r->data[r->pos++];
    return 0;
}

static int read_leb_u32(reader_t *r, uint32_t *out) {
    uint32_t value = 0;
    unsigned shift = 0;
    for (int i = 0; i < 5; i++) {
        uint8_t byte = 0;
        if (read_u8(r, &byte) != 0) return -1;
        value |= (uint32_t)(byte & 0x7f) << shift;
        if ((byte & 0x80) == 0) {
            *out = value;
            return 0;
        }
        shift += 7;
    }
    return -1;
}

typedef struct {
    const uint8_t *bytes;
    size_t length;
} wasm_name_t;

static int read_name(reader_t *r, wasm_name_t *out) {
    uint32_t length = 0;
    if (read_leb_u32(r, &length) != 0 || length > MAX_IMPORT_NAME) return -1;
    if (r->len - r->pos < length) return -1;
    out->bytes = r->data + r->pos;
    out->length = length;
    r->pos += length;
    return 0;
}

static bool name_is(const wasm_name_t *name, const char *text) {
    size_t length = strlen(text);
    return name->length == length && memcmp(name->bytes, text, length) == 0;
}

static int skip_limits(reader_t *r) {
    uint32_t flags = 0;
    uint32_t value = 0;
    if (read_leb_u32(r, &flags) != 0) return -1;
    /* 0x00 min only, 0x01 min+max. Shared or 64-bit memories are not supported. */
    if (flags != 0 && flags != 1) return -1;
    if (read_leb_u32(r, &value) != 0) return -1;
    if (flags == 1 && read_leb_u32(r, &value) != 0) return -1;
    return 0;
}

static bool import_allowed(const wasm_name_t *module, const wasm_name_t *name) {
    for (size_t i = 0; i < sizeof(k_allowed) / sizeof(k_allowed[0]); i++) {
        if (name_is(module, k_allowed[i].module) && name_is(name, k_allowed[i].name)) return true;
    }
    return false;
}

static int check_import(reader_t *r, char *reason, size_t reason_size) {
    wasm_name_t module;
    wasm_name_t name;
    uint8_t kind = 0;
    if (read_name(r, &module) != 0 || read_name(r, &name) != 0) return -1;
    if (read_u8(r, &kind) != 0) return -1;

    uint32_t value = 0;
    switch (kind) {
    case 0x00:
        if (read_leb_u32(r, &value) != 0) return -1;
        break;
    case 0x01: {
        uint8_t elem_type = 0;
        if (read_u8(r, &elem_type) != 0 || skip_limits(r) != 0) return -1;
        break;
    }
    case 0x02:
        if (skip_limits(r) != 0) return -1;
        break;
    case 0x03: {
        uint8_t val_type = 0;
        uint8_t mutability = 0;
        if (read_u8(r, &val_type) != 0 || read_u8(r, &mutability) != 0) return -1;
        break;
    }
    default:
        return -1;
    }

    if (!import_allowed(&module, &name)) {
        if (reason && reason_size) {
            snprintf(reason, reason_size, "denied_import: %.*s.%.*s", (int)module.length,
                     (const char *)module.bytes, (int)name.length, (const char *)name.bytes);
        }
        return -2;
    }
    return 0;
}

static int check_import_section(const uint8_t *payload, size_t size, char *reason,
                                size_t reason_size) {
    reader_t r = {.data = payload, .len = size, .pos = 0};
    uint32_t count = 0;
    if (read_leb_u32(&r, &count) != 0 || count > MAX_IMPORTS) return -1;
    for (uint32_t i = 0; i < count; i++) {
        int rc = check_import(&r, reason, reason_size);
        if (rc != 0) return rc;
    }
    if (r.pos != size) return -1;
    return 0;
}

int edge_wasm_imports_validate(const void *wasm, size_t len, char *reason, size_t reason_size) {
    if (reason && reason_size) reason[0] = '\0';
    if (!wasm || len < WASM_HEADER_SIZE) {
        if (reason && reason_size) snprintf(reason, reason_size, "invalid_wasm");
        return -1;
    }

    const uint8_t *data = (const uint8_t *)wasm;
    if (data[0] != 0x00 || data[1] != 0x61 || data[2] != 0x73 || data[3] != 0x6d ||
        data[4] != 0x01 || data[5] != 0x00 || data[6] != 0x00 || data[7] != 0x00) {
        if (reason && reason_size) snprintf(reason, reason_size, "invalid_wasm");
        return -1;
    }

    reader_t r = {.data = data, .len = len, .pos = WASM_HEADER_SIZE};
    while (r.pos < r.len) {
        uint8_t section_id = 0;
        uint32_t section_size = 0;
        if (read_u8(&r, &section_id) != 0 || read_leb_u32(&r, &section_size) != 0 ||
            r.len - r.pos < section_size) {
            if (reason && reason_size) snprintf(reason, reason_size, "invalid_wasm");
            return -1;
        }
        const uint8_t *payload = r.data + r.pos;
        r.pos += section_size;

        if (section_id == 2) {
            int rc = check_import_section(payload, section_size, reason, reason_size);
            if (rc == -2) return -1;
            if (rc != 0) {
                if (reason && reason_size) snprintf(reason, reason_size, "invalid_wasm");
                return -1;
            }
        }
    }
    return 0;
}
