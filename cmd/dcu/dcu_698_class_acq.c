#include "dcu_698_object_internal.h"

#include <string.h>
#include <uv.h>

#include "dcu_discovery.h"
#include "dcu_poller.h"
#include "dlt698_codec.h"
#include "edge_log.h"

static int _put_u8(uint8_t *buf, size_t cap, size_t *off, uint8_t v);
static int _put_u16(uint8_t *buf, size_t cap, size_t *off, uint16_t v);
static int _put_enum(uint8_t *buf, size_t cap, size_t *off, uint8_t v);

int dcu_698_acq_6012_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result) {
    dcu_poller_t *poller = (dcu_poller_t *)obj->instance;
    if (!poller) return EDGE_SLAVE_SRC_TEMPORARY;

    if (method_id == 1) {
        /* Class 10 复位只清运行态；配置与落库要等 129 Clear。 */
        for (int i = 0; i < poller->task_count; i++) {
            poller->tasks[i].cursor_meter_idx = 0;
            poller->tasks[i].last_run_time = 0;
            memset(&poller->tasks[i].monitor, 0, sizeof(poller->tasks[i].monitor));
        }
        if (result) *(int *)result = 0;
        return 0;
    }

    const dcu_task_op_t *op = (const dcu_task_op_t *)param;
    if (!op || op->method != method_id) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    const router_sink_ops_t *sink = dcu_698_internal_sink_ops();
    if (!sink || !sink->task_apply) return EDGE_SLAVE_SRC_TEMPORARY;
    return sink->task_apply(sink->ctx, poller, op);
}

int dcu_698_acq_6014_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result) {
    (void)result;
    dcu_poller_t *poller = (dcu_poller_t *)obj->instance;
    if (!poller) return EDGE_SLAVE_SRC_TEMPORARY;
    const dcu_scheme_op_t *op = (const dcu_scheme_op_t *)param;
    if (!op || op->method != method_id) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    const router_sink_ops_t *sink = dcu_698_internal_sink_ops();
    if (!sink || !sink->scheme_apply) return EDGE_SLAVE_SRC_TEMPORARY;
    return sink->scheme_apply(sink->ctx, poller, op);
}

int dcu_698_acq_6002_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          void *out_val) {
    (void)elem_idx;
    dcu_discovery_t *disc = (dcu_discovery_t *)obj->instance;
    if (!disc || !out_val) return -1;

    uint8_t status = 0;
    uint16_t count = 0;
    dcu_discovery_snapshot(disc, &status, &count);
    switch (attr_id) {
        case 3: /* Class 11 当前元素个数 */
        case 6: /* 表 A.7 所有搜表结果记录数 */
            *(uint16_t *)out_val = count;
            return 0;
        case 10: /* 表 A.7 空闲/搜表中 */
            *(uint8_t *)out_val = status;
            return 0;
        default:
            return -2;
    }
}

int dcu_698_acq_6002_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result) {
    dcu_discovery_t *disc = (dcu_discovery_t *)obj->instance;
    if (!disc) return EDGE_SLAVE_SRC_TEMPORARY;

    switch (method_id) {
        case 127: {
            if (!param) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            uint32_t duration_min = *(const uint32_t *)param;
            int ret = dcu_discovery_start(disc, duration_min);
            if (result) *(int *)result = ret;
            return ret;
        }
        case 128:
            dcu_discovery_clear(disc);
            return 0;
        case 129:
            /* 产品没有跨台区结果集，清空操作没有对象可清；不是把 A2 一起清掉。 */
            return 0;
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
}

int dcu_698_acq_6002_encode(const dcu_698_object_t *obj, uint32_t oad, uint8_t *buf, size_t cap,
                            size_t *len) {
    if (!obj || !buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    dcu_discovery_t *disc = (dcu_discovery_t *)obj->instance;
    if (!disc) return EDGE_SLAVE_SRC_TEMPORARY;

    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t status = 0;
    uint16_t count = 0;
    dcu_discovery_snapshot(disc, &status, &count);

    size_t off = 0;
    switch (attr_id) {
        case 2:
            /* 产品没有下行搜表驱动，结果集只能为空。非空结果缺 6003 的
             * 所属采集器/信号品质等硬件字段，回暂时失效，不编造。 */
            if (count != 0) return EDGE_SLAVE_SRC_TEMPORARY;
            if (_put_u8(buf, cap, &off, DLT698_TAG_ARRAY) != 0 || _put_u8(buf, cap, &off, 0) != 0) {
                return EDGE_SLAVE_SRC_TEMPORARY;
            }
            break;
        case 3:
        case 6:
            if (_put_u16(buf, cap, &off, count) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
            break;
        case 10:
            if (_put_enum(buf, cap, &off, status) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
            break;
        default:
            return dcu_698_oi_defines_attr(obj->oi, obj->class_id, attr_id)
                       ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                       : EDGE_SLAVE_SRC_UNDEFINED;
    }
    *len = off;
    return 0;
}

static int _put_u8(uint8_t *buf, size_t cap, size_t *off, uint8_t v) {
    if (*off + 1 > cap) return -1;
    buf[(*off)++] = v;
    return 0;
}

static int _put_u16(uint8_t *buf, size_t cap, size_t *off, uint16_t v) {
    if (*off + 3 > cap) return -1;
    buf[(*off)++] = DLT698_TAG_LONG_UNSIGNED;
    buf[(*off)++] = (uint8_t)(v >> 8);
    buf[(*off)++] = (uint8_t)v;
    return 0;
}

static int _put_enum(uint8_t *buf, size_t cap, size_t *off, uint8_t v) {
    if (*off + 2 > cap) return -1;
    buf[(*off)++] = DLT698_TAG_ENUM;
    buf[(*off)++] = v;
    return 0;
}

static int _put_octets(uint8_t *buf, size_t cap, size_t *off, const char *s) {
    size_t n = s ? strlen(s) : 0;
    if (n > 64 || *off + 2 + n > cap) return -1;
    buf[(*off)++] = DLT698_TAG_OCTET_STRING;
    buf[(*off)++] = (uint8_t)n;
    if (n > 0) memcpy(buf + *off, s, n);
    *off += n;
    return 0;
}

static int _put_oad_port(uint8_t *buf, size_t cap, size_t *off, uint8_t port) {
    if (*off + 5 > cap) return -1;
    buf[(*off)++] = DLT698_TAG_OAD;
    buf[(*off)++] = 0;
    buf[(*off)++] = 0;
    buf[(*off)++] = 0;
    buf[(*off)++] = port;
    return 0;
}

/* 6001 A2：structure{配置序号, Basic, Extended, Annex}。 */
static int _encode_unit(uint8_t *buf, size_t cap, size_t *off, const production_meter_t *m) {
    if (_put_u8(buf, cap, off, DLT698_TAG_STRUCTURE) != 0 ||
        _put_u8(buf, cap, off, 4) != 0 || _put_u16(buf, cap, off, m->config_seq) != 0) {
        return -1;
    }

    if (_put_u8(buf, cap, off, DLT698_TAG_STRUCTURE) != 0 ||
        _put_u8(buf, cap, off, 10) != 0 || _put_octets(buf, cap, off, m->address) != 0 ||
        _put_enum(buf, cap, off, m->baud_code) != 0 ||
        _put_enum(buf, cap, off, m->protocol_code) != 0 ||
        _put_oad_port(buf, cap, off, m->channel_id) != 0 ||
        _put_octets(buf, cap, off, m->password) != 0 ||
        _put_u8(buf, cap, off, DLT698_TAG_UNSIGNED) != 0 ||
        _put_u8(buf, cap, off, m->tariff_count) != 0 ||
        _put_u8(buf, cap, off, DLT698_TAG_UNSIGNED) != 0 ||
        _put_u8(buf, cap, off, m->user_type) != 0 ||
        _put_enum(buf, cap, off, m->wiring_mode) != 0 ||
        _put_u16(buf, cap, off, m->rated_voltage) != 0 ||
        _put_u16(buf, cap, off, m->rated_current) != 0) {
        return -1;
    }

    if (_put_u8(buf, cap, off, DLT698_TAG_STRUCTURE) != 0 ||
        _put_u8(buf, cap, off, 4) != 0 ||
        _put_octets(buf, cap, off, m->collector_addr) != 0 ||
        _put_octets(buf, cap, off, m->asset_no) != 0 ||
        _put_u16(buf, cap, off, (uint16_t)m->pt_ratio) != 0 ||
        _put_u16(buf, cap, off, (uint16_t)m->ct_ratio) != 0) {
        return -1;
    }

    /* 附属信息数组：产品没有属性值存储，回空数组（标准允许）。 */
    return _put_u8(buf, cap, off, DLT698_TAG_ARRAY) == 0 &&
                   _put_u8(buf, cap, off, 0) == 0
               ? 0
               : -1;
}

int dcu_698_archive_encode_axdr(const dcu_inventory_t *inv, uint32_t oad, uint8_t *buf, size_t cap,
                                size_t *len) {
    if (!inv || !buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem = (uint8_t)(oad & 0xFF);
    if (attr_id != 2) {
        /* 类定义了但没广告 → DAR 5；类里没有这个属性号 → DAR 4。 */
        return dcu_698_class_defines_attr((uint8_t)(oi == OI_METER_ARCHIVE ? IC_ACQUISITION
                                                                           : IC_PARAM),
                                          attr_id)
                   ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                   : EDGE_SLAVE_SRC_UNDEFINED;
    }

    size_t off = 0;
    if (oi == OI_METER_ARCHIVE) {
        /* A-XDR 数组长度只有一个字节：超过 255 只表必须如实拒绝，不能回绕。 */
        if (inv->count > 255) return EDGE_SLAVE_SRC_TEMPORARY;
        if (_put_u8(buf, cap, &off, DLT698_TAG_ARRAY) != 0 ||
            _put_u8(buf, cap, &off, (uint8_t)inv->count) != 0) {
            return EDGE_SLAVE_SRC_TEMPORARY; /* 40 只表的完整结构放不进 2048 字节：如实拒绝 */
        }
        for (uint16_t i = 0; i < inv->count; i++) {
            if (_encode_unit(buf, cap, &off, &inv->meters[i]) != 0) {
                return EDGE_SLAVE_SRC_TEMPORARY;
            }
        }
        *len = off;
        return 0;
    }
    if (oi == OI_ARCHIVE_UNIT) {
        const production_meter_t *m = elem ? dcu_inventory_by_seq(inv, elem)
                                           : (inv->count ? &inv->meters[0] : NULL);
        if (!m) return EDGE_SLAVE_SRC_NOT_FOUND;
        if (_encode_unit(buf, cap, &off, m) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
        *len = off;
        return 0;
    }
    return EDGE_SLAVE_SRC_NOT_FOUND;
}

static int _save_unit(const router_sink_ops_t *sink, const production_meter_t *m) {
    if (!sink || !sink->archive_save || !m) return EDGE_SLAVE_SRC_TEMPORARY;
    return sink->archive_save(sink->ctx, m) == 0 ? 0 : EDGE_SLAVE_SRC_TEMPORARY;
}

static int _delete_addr(const router_sink_ops_t *sink, const char *address) {
    if (!sink || !sink->archive_delete) return EDGE_SLAVE_SRC_TEMPORARY;
    return sink->archive_delete(sink->ctx, address) == 0 ? 0 : EDGE_SLAVE_SRC_TEMPORARY;
}

static int _add_unit(dcu_inventory_t *inv, const router_sink_ops_t *sink,
                     const dcu_archive_unit_t *unit) {
    int rc = dcu_inventory_add_unit(inv, unit);
    if (rc == -2 || rc == -3) return EDGE_SLAVE_SRC_DENIED;
    if (rc == -4) return EDGE_SLAVE_SRC_TEMPORARY;
    if (rc != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    const production_meter_t *m = &inv->meters[inv->count - 1];
    int save = _save_unit(sink, m);
    if (save != 0) {
        dcu_inventory_remove_seq(inv, m->config_seq); /* 落库失败不留 RAM 半态 */
        return save;
    }
    return 0;
}

static int _add_batch(dcu_inventory_t *inv, const router_sink_ops_t *sink,
                      const dcu_archive_op_t *op) {
    if (op->unit_count == 0 || op->unit_count > DCU_ARCHIVE_BATCH_MAX) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    for (uint16_t i = 0; i < op->unit_count; i++) {
        int rc = _add_unit(inv, sink, &op->units[i]);
        if (rc != 0) {
            /* 回滚本批已经落库/落 RAM 的单元，保证批量要么全成要么全不成。 */
            for (uint16_t k = 0; k < i; k++) {
                _delete_addr(sink, op->units[k].address);
                dcu_inventory_remove(inv, op->units[k].address);
            }
            return rc;
        }
    }
    return 0;
}

int dcu_698_acq_6000_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result) {
    (void)result;
    dcu_inventory_t *inv = (dcu_inventory_t *)obj->instance;
    const dcu_archive_op_t *op = (const dcu_archive_op_t *)param;
    const router_sink_ops_t *sink = dcu_698_internal_sink_ops();
    if (!inv || !op) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    switch (method_id) {
        case 127:
            if (op->unit_count != 1) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (op->has_annex) return EDGE_SLAVE_SRC_TEMPORARY;
            return _add_unit(inv, sink, &op->units[0]);
        case 128:
            return _add_batch(inv, sink, op);
        case 129: {
            if (op->unit_count != 1) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            const production_meter_t *old = dcu_inventory_by_seq(inv, op->config_seq);
            if (!old) return EDGE_SLAVE_SRC_NOT_FOUND;
            char old_address[MAX_ADDR_LEN];
            strncpy(old_address, old->address, sizeof(old_address) - 1);
            old_address[sizeof(old_address) - 1] = '\0';
            int rc = dcu_inventory_update_basic(inv, op->config_seq, &op->units[0]);
            if (rc == -2) return EDGE_SLAVE_SRC_DENIED;
            if (rc != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            const production_meter_t *m = dcu_inventory_by_seq(inv, op->config_seq);
            int save = _save_unit(sink, m);
            if (save != 0) return save;
            if (strcmp(old_address, m->address) != 0) _delete_addr(sink, old_address);
            return 0;
        }
        case 130: {
            if (op->unit_count != 1) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            int rc = dcu_inventory_update_extended(inv, op->config_seq, &op->units[0],
                                                   op->has_annex);
            if (rc == -1) return EDGE_SLAVE_SRC_NOT_FOUND;
            if (rc == -2) return EDGE_SLAVE_SRC_TEMPORARY;
            if (rc != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            return _save_unit(sink, dcu_inventory_by_seq(inv, op->config_seq));
        }
        case 131: {
            const production_meter_t *m = dcu_inventory_by_seq(inv, op->config_seq);
            if (!m) return EDGE_SLAVE_SRC_NOT_FOUND;
            char address[MAX_ADDR_LEN];
            strncpy(address, m->address, sizeof(address) - 1);
            address[sizeof(address) - 1] = '\0';
            int del = _delete_addr(sink, address);
            if (del != 0) return del;
            dcu_inventory_remove_seq(inv, op->config_seq);
            return 0;
        }
        case 132:
        case 133:
            if (op->unit_count != 1) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (!dcu_inventory_get(inv, op->units[0].address)) return EDGE_SLAVE_SRC_NOT_FOUND;
            {
                int del = _delete_addr(sink, op->units[0].address);
                if (del != 0) return del;
            }
            dcu_inventory_remove(inv, op->units[0].address);
            return 0;
        case 134: {
            if (!sink || !sink->archive_clear) return EDGE_SLAVE_SRC_TEMPORARY;
            if (sink->archive_clear(sink->ctx) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
            dcu_inventory_clear(inv);
            return 0;
        }
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
}
