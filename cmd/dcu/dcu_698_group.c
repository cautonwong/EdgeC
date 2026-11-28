/**
 * @file dcu_698_group.c
 * @brief Class 23 总加组（2301..2308）领域引擎与滑差/A-XDR 编码实现
 */

#include "dcu_698_group.h"

#include <string.h>

#include "dlt698_codec.h"

#define DCU_GROUP_AXDR_TAG_TSA 85u

static dcu_group_state_t g_groups[DCU_GROUP_COUNT];

static bool _member_matches(const dcu_group_member_t *m, uint32_t slot_id, const uint8_t *tsa,
                            uint8_t tsa_len) {
    if (tsa && tsa_len > 0) {
        return m->tsa_len == tsa_len && memcmp(m->tsa, tsa, tsa_len) == 0;
    }
    return m->tsa_len == 0 && m->slot_id == slot_id;
}

static int _put_tagged_u8(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, uint8_t v) {
    if (*off + 2 > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = v;
    return 0;
}

static int _put_long64(uint8_t *buf, size_t cap, size_t *off, int64_t val) {
    if (*off + 9 > cap) return -1;
    buf[(*off)++] = DLT698_TAG_LONG64;
    uint64_t u = (uint64_t)val;
    for (int shift = 56; shift >= 0; shift -= 8) {
        buf[(*off)++] = (uint8_t)(u >> shift);
    }
    return 0;
}

static int _put_member_tsa(uint8_t *buf, size_t cap, size_t *off, const dcu_group_member_t *m) {
    uint8_t fallback[6];
    const uint8_t *bytes = m->tsa;
    uint8_t len = m->tsa_len;
    if (len == 0) {
        /* 仅提供 slot_id 时生成 6 字节 TSA（首字节 0x05 表示 6 字节地址长度-1） */
        fallback[0] = 0x05u;
        fallback[1] = 0x00u;
        fallback[2] = (uint8_t)(m->slot_id >> 24);
        fallback[3] = (uint8_t)(m->slot_id >> 16);
        fallback[4] = (uint8_t)(m->slot_id >> 8);
        fallback[5] = (uint8_t)(m->slot_id);
        bytes = fallback;
        len = (uint8_t)sizeof(fallback);
    }
    if (len < 2 || len > DCU_GROUP_TSA_MAX) return -1;
    if (*off + 2 + len > cap) return -1;
    buf[(*off)++] = DCU_GROUP_AXDR_TAG_TSA;
    buf[(*off)++] = len;
    memcpy(buf + *off, bytes, len);
    *off += len;
    return 0;
}

static int _encode_a2(const dcu_group_state_t *g, uint8_t *buf, size_t cap, size_t *off) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_ARRAY, g->member_count) != 0) return -1;
    for (uint8_t i = 0; i < g->member_count; i++) {
        const dcu_group_member_t *m = &g->members[i];
        if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 3) != 0 ||
            _put_member_tsa(buf, cap, off, m) != 0 ||
            _put_tagged_u8(buf, cap, off, DLT698_TAG_ENUM, m->calc_flag) != 0 ||
            _put_tagged_u8(buf, cap, off, DLT698_TAG_ENUM, m->add_sub_flag) != 0) {
            return -1;
        }
    }
    return 0;
}

static int _encode_energy_array(int64_t total, uint8_t *buf, size_t cap, size_t *off) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_ARRAY, 1) != 0) return -1;
    return _put_long64(buf, cap, off, total);
}

void dcu_698_group_init(void) { memset(g_groups, 0, sizeof(g_groups)); }

int dcu_698_group_add_member(uint8_t group_idx, const dcu_group_member_t *member) {
    if (group_idx >= DCU_GROUP_COUNT || !member) return -1;
    if (member->add_sub_flag > 1 || member->calc_flag > 1) return -1;
    if (member->tsa_len != 0 &&
        (member->tsa_len < 2 || member->tsa_len > DCU_GROUP_TSA_MAX)) {
        return -1;
    }

    dcu_group_state_t *g = &g_groups[group_idx];
    const uint8_t *tsa = member->tsa_len > 0 ? member->tsa : NULL;
    for (uint8_t i = 0; i < g->member_count; i++) {
        if (_member_matches(&g->members[i], member->slot_id, tsa, member->tsa_len)) {
            g->members[i] = *member;
            return 0;
        }
    }
    if (g->member_count >= DCU_GROUP_MAX_MEMBERS) return -1;
    g->members[g->member_count++] = *member;
    return 0;
}

int dcu_698_group_delete_member(uint8_t group_idx, uint32_t slot_id, const uint8_t *tsa,
                                uint8_t tsa_len) {
    if (group_idx >= DCU_GROUP_COUNT) return -1;
    dcu_group_state_t *g = &g_groups[group_idx];
    for (uint8_t i = 0; i < g->member_count; i++) {
        if (_member_matches(&g->members[i], slot_id, tsa, tsa_len)) {
            for (uint8_t j = i + 1; j < g->member_count; j++) {
                g->members[j - 1] = g->members[j];
            }
            g->member_count--;
            memset(&g->members[g->member_count], 0, sizeof(g->members[0]));
            return 0;
        }
    }
    return -1;
}

int dcu_698_group_clear(uint8_t group_idx) {
    if (group_idx >= DCU_GROUP_COUNT) return -1;
    memset(&g_groups[group_idx], 0, sizeof(g_groups[group_idx]));
    return 0;
}

int dcu_698_group_sample(uint8_t group_idx, dcu_group_read_fn read_fn, void *ctx, uint64_t now_s) {
    if (group_idx >= DCU_GROUP_COUNT || !read_fn) return -1;
    dcu_group_state_t *g = &g_groups[group_idx];

    int64_t sum_p = 0;
    int64_t sum_q = 0;
    int64_t sum_pae = 0;
    int64_t sum_nae = 0;
    int64_t sum_pre = 0;
    int64_t sum_nre = 0;
    uint8_t valid = 0;

    for (uint8_t i = 0; i < g->member_count; i++) {
        const dcu_group_member_t *m = &g->members[i];
        if (m->calc_flag == 0) continue;

        dcu_group_member_reading_t r;
        memset(&r, 0, sizeof(r));
        if (read_fn(ctx, m, &r) != 0) continue;

        int64_t sign = (m->add_sub_flag == 1) ? -1 : 1;
        sum_p += sign * r.active_power;
        sum_q += sign * r.reactive_power;
        sum_pae += sign * r.pos_active_energy;
        sum_nae += sign * r.neg_active_energy;
        sum_pre += sign * r.pos_reactive_energy;
        sum_nre += sign * r.neg_reactive_energy;
        valid++;
    }

    if (valid == 0) return -1;

    g->active_power = sum_p;
    g->reactive_power = sum_q;
    g->pos_active_energy = sum_pae;
    g->neg_active_energy = sum_nae;
    g->pos_reactive_energy = sum_pre;
    g->neg_reactive_energy = sum_nre;

    g->slip_active_hist[g->slip_head] = sum_p;
    g->slip_reactive_hist[g->slip_head] = sum_q;
    g->slip_head = (uint8_t)((g->slip_head + 1u) % DCU_GROUP_SLIP_WINDOW);
    if (g->slip_count < DCU_GROUP_SLIP_WINDOW) g->slip_count++;

    int64_t slip_p_sum = 0;
    int64_t slip_q_sum = 0;
    for (uint8_t i = 0; i < g->slip_count; i++) {
        slip_p_sum += g->slip_active_hist[i];
        slip_q_sum += g->slip_reactive_hist[i];
    }
    g->slip_active_power = slip_p_sum / (int64_t)g->slip_count;
    g->slip_reactive_power = slip_q_sum / (int64_t)g->slip_count;
    g->last_sample_s = now_s;
    g->has_sample = true;
    return 0;
}

const dcu_group_state_t *dcu_698_group_get(uint8_t group_idx) {
    if (group_idx >= DCU_GROUP_COUNT) return NULL;
    return &g_groups[group_idx];
}

int dcu_698_group_encode_attr(uint8_t group_idx, uint8_t attr_id, uint8_t *buf, size_t cap,
                              size_t *out_len) {
    if (group_idx >= DCU_GROUP_COUNT || !buf || !out_len) return -1;
    const dcu_group_state_t *g = &g_groups[group_idx];
    size_t off = 0;
    int rc = -1;

    switch (attr_id) {
        case 2:
            rc = _encode_a2(g, buf, cap, &off);
            break;
        case 3:
            if (!g->has_sample) return -1;
            rc = _put_long64(buf, cap, &off, g->active_power);
            break;
        case 4:
            if (!g->has_sample) return -1;
            rc = _put_long64(buf, cap, &off, g->reactive_power);
            break;
        case 5:
            if (!g->has_sample) return -1;
            rc = _put_long64(buf, cap, &off, g->slip_active_power);
            break;
        case 6:
            if (!g->has_sample) return -1;
            rc = _put_long64(buf, cap, &off, g->slip_reactive_power);
            break;
        case 7:
            if (!g->has_sample) return -1;
            rc = _encode_energy_array(g->pos_active_energy, buf, cap, &off);
            break;
        case 8:
            if (!g->has_sample) return -1;
            rc = _encode_energy_array(g->neg_active_energy, buf, cap, &off);
            break;
        case 9:
            if (!g->has_sample) return -1;
            rc = _encode_energy_array(g->pos_reactive_energy, buf, cap, &off);
            break;
        case 10:
            if (!g->has_sample) return -1;
            rc = _encode_energy_array(g->neg_reactive_energy, buf, cap, &off);
            break;
        default:
            return -1;
    }

    if (rc != 0) return -1;
    *out_len = off;
    return 0;
}
