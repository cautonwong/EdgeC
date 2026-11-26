#include "dcu_698_axdr_internal.h"

#include <string.h>

#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_698_rules.h"
#include "dcu_poller.h"
#include "dcu_scheme_store.h"
#include "dlt698_codec.h"
#include "edge_slave_plane.h"

#define DCU_698_TAG_ROAD 0x52

static int _take_oad_raw(axdr_cursor_t *c, uint32_t *out) {
    const uint8_t *p = NULL;
    if (!_take(c, 4, &p)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (out) {
        *out = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) |
               (uint32_t)p[3];
    }
    return 0;
}

static int _take_event_content(axdr_cursor_t *c, dcu_event_capture_t capture_type,
                               dcu_event_scheme_cfg_t *u) {
    if (capture_type == DCU_EVENT_CAPTURE_NOTIFY_ALL) {
        const uint8_t *p = NULL;
        if (!_take(c, 1, &p) || p[0] != DLT698_TAG_NULL) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        return 0;
    }
    uint8_t count = 0;
    int rc = _take_array(c, &count);
    if (rc != 0) return rc;
    if (count > DCU_MAX_EVENT_ROADS) return EDGE_SLAVE_SRC_TEMPORARY;
    u->road_count = count;
    for (uint8_t i = 0; i < count; i++) {
        const uint8_t *p = NULL;
        if (!_take(c, 1, &p) || p[0] != DCU_698_TAG_ROAD) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        dcu_event_road_t *road = &u->roads[i];
        rc = _take_oad_raw(c, &road->oad);
        if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &road->related_count);
        if (rc != 0) return rc;
        if (road->related_count > DCU_ROAD_RELATED_MAX) return EDGE_SLAVE_SRC_TEMPORARY;
        for (uint8_t r = 0; r < road->related_count; r++) {
            rc = _take_oad_raw(c, &road->related[r]);
            if (rc != 0) return rc;
        }
    }
    return 0;
}

static int _decode_event_unit(axdr_cursor_t *c, dcu_event_scheme_cfg_t *u) {
    uint8_t report = 0;
    uint8_t capture_type = 0;
    int rc = _take_head(c, DLT698_TAG_STRUCTURE, 5);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &u->scheme_id);
    if (rc == 0) rc = _take_head(c, DLT698_TAG_STRUCTURE, 2);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &capture_type);
    if (rc == 0 && capture_type > DCU_EVENT_CAPTURE_NOTIFY_LIST)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    u->capture_type = (dcu_event_capture_t)capture_type;
    if (rc == 0) rc = _take_event_content(c, u->capture_type, u);
    if (rc == 0) rc = dcu_698_axdr_take_ms(c, &u->ms_choice);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_BOOLEAN, &report);
    if (rc == 0) rc = _take_u16(c, DLT698_TAG_LONG_UNSIGNED, &u->storage_depth);
    if (rc != 0) return rc;
    u->report_flag = report != 0;
    return 0;
}

int dcu_698_decode_event_scheme_op(uint8_t method, const uint8_t *buf, size_t len,
                                   dcu_event_scheme_op_t *out) {
    if (!buf || !out || len == 0 || len > UINT16_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    axdr_cursor_t c = {.p = buf, .len = len, .off = 0};
    memset(out, 0, sizeof(*out));
    out->method = method;
    uint8_t count = 0;
    int rc = 0;
    switch (method) {
        case 127:
            rc = _take_array(&c, &count);
            if (rc != 0) return rc;
            if (count == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (count > DCU_MAX_EVENT_SCHEMES) return EDGE_SLAVE_SRC_TEMPORARY;
            for (uint8_t i = 0; i < count; i++) {
                rc = _decode_event_unit(&c, &out->units[i]);
                if (rc != 0) return rc;
                for (uint8_t j = 0; j < i; j++) {
                    if (out->units[j].scheme_id == out->units[i].scheme_id)
                        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
                }
            }
            out->unit_count = count;
            break;
        case 128:
            rc = _take_array(&c, &count);
            if (rc != 0) return rc;
            if (count == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (count > DCU_MAX_EVENT_SCHEMES) return EDGE_SLAVE_SRC_TEMPORARY;
            for (uint8_t i = 0; i < count; i++) {
                rc = _take_u8(&c, DLT698_TAG_UNSIGNED, &out->ids[i]);
                if (rc != 0) return rc;
            }
            out->id_count = count;
            break;
        case 129: {
            const uint8_t *p = NULL;
            if (!_take(&c, 1, &p) || p[0] != DLT698_TAG_NULL)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            break;
        }
        case 130: {
            uint8_t flag = 0;
            rc = _take_head(&c, DLT698_TAG_STRUCTURE, 2);
            if (rc == 0) rc = _take_u8(&c, DLT698_TAG_UNSIGNED, &out->scheme_id);
            if (rc == 0) rc = _take_u8(&c, DLT698_TAG_BOOLEAN, &flag);
            if (rc != 0) return rc;
            out->report_flag = flag != 0;
            break;
        }
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
    return _time_tag_absent(buf, c.off, c.len) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

static int _put_oad_raw(uint8_t *buf, size_t cap, size_t *off, uint32_t oad) {
    if (*off + 4 > cap) return -1;
    buf[(*off)++] = (uint8_t)(oad >> 24);
    buf[(*off)++] = (uint8_t)(oad >> 16);
    buf[(*off)++] = (uint8_t)(oad >> 8);
    buf[(*off)++] = (uint8_t)oad;
    return 0;
}

static int _encode_event_content(uint8_t *buf, size_t cap, size_t *off,
                                 const dcu_event_scheme_cfg_t *u) {
    if (u->capture_type == DCU_EVENT_CAPTURE_NOTIFY_ALL)
        return _put_raw_u8(buf, cap, off, DLT698_TAG_NULL);
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_ARRAY, u->road_count) != 0) return -1;
    for (uint8_t i = 0; i < u->road_count; i++) {
        const dcu_event_road_t *road = &u->roads[i];
        if (_put_raw_u8(buf, cap, off, DCU_698_TAG_ROAD) != 0 ||
            _put_oad_raw(buf, cap, off, road->oad) != 0 ||
            _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, road->related_count) != 0) {
            return -1;
        }
        for (uint8_t r = 0; r < road->related_count; r++) {
            if (_put_oad_raw(buf, cap, off, road->related[r]) != 0) return -1;
        }
    }
    return 0;
}

static int _encode_event_unit(uint8_t *buf, size_t cap, size_t *off,
                              const dcu_event_scheme_cfg_t *u) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 5) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, u->scheme_id) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 2) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, (uint8_t)u->capture_type) != 0) {
        return -1;
    }
    if (_encode_event_content(buf, cap, off, u) != 0) return -1;
    if (_put_raw_u8(buf, cap, off, DCU_698_TAG_MS) != 0 ||
        _put_raw_u8(buf, cap, off, u->ms_choice) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_BOOLEAN, u->report_flag ? 1 : 0) != 0 ||
        _put_tagged_u16(buf, cap, off, DLT698_TAG_LONG_UNSIGNED, u->storage_depth) != 0) {
        return -1;
    }
    return 0;
}

int dcu_698_event_scheme_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem = (uint8_t)(oad & 0xFF);
    size_t off = 0;
    if (attr_id != 2) {
        return dcu_698_class_defines_attr(IC_COLLECTION, attr_id)
                   ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                   : EDGE_SLAVE_SRC_UNDEFINED;
    }
    if (oi == OI_EVENT_SCHEME) {
        int count = dcu_scheme_store_event_count();
        if (count < 0) return EDGE_SLAVE_SRC_TEMPORARY;
        if (count > 255) return EDGE_SLAVE_SRC_TEMPORARY;
        if (_put_tagged_u8(buf, cap, &off, DLT698_TAG_ARRAY, (uint8_t)count) != 0)
            return EDGE_SLAVE_SRC_TEMPORARY;
        for (int i = 0; i < count; i++) {
            dcu_event_scheme_cfg_t u;
            int rc = dcu_scheme_store_event_at((uint16_t)i, &u);
            if (rc != 0) return rc;
            if (_encode_event_unit(buf, cap, &off, &u) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
        }
    } else {
        dcu_event_scheme_cfg_t u;
        int rc = elem ? dcu_scheme_store_event_get(elem, &u)
                      : dcu_scheme_store_event_at(0, &u);
        if (rc != 0) return rc;
        if (_encode_event_unit(buf, cap, &off, &u) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    }
    *len = off;
    return 0;
}

static int _take_tsa(axdr_cursor_t *c, dcu_tsa_t *out) {
    const uint8_t *p = NULL;
    if (!_take(c, 1, &p) || p[0] < 2 || p[0] > DCU_TSA_MAX)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint8_t n = p[0];
    if (!_take(c, n, &p)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    out->len = n;
    memcpy(out->bytes, p, n);
    return 0;
}

static int _take_octets(axdr_cursor_t *c, uint8_t *dst, uint16_t cap, uint16_t *out_len) {
    const uint8_t *p = NULL;
    if (!_take(c, 1, &p) || p[0] != DLT698_TAG_OCTET_STRING)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (!_take(c, 1, &p)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint8_t n = p[0];
    if (n > cap) return EDGE_SLAVE_SRC_TEMPORARY;
    if (!_take(c, n, &p)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (n > 0) memcpy(dst, p, n);
    *out_len = n;
    return 0;
}

static int _decode_tp_ctrl(axdr_cursor_t *c, dcu_transparent_ctrl_t *ctrl) {
    uint8_t flag = 0;
    int rc = _take_head(c, DLT698_TAG_STRUCTURE, 4);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_BOOLEAN, &flag);
    if (rc == 0) rc = _take_u16(c, DLT698_TAG_LONG_UNSIGNED, &ctrl->wait_timeout_s);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_ENUM, &ctrl->compare_mode);
    if (rc == 0) rc = _take_head(c, DLT698_TAG_STRUCTURE, 3);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &ctrl->feature_byte);
    if (rc == 0) rc = _take_u16(c, DLT698_TAG_LONG_UNSIGNED, &ctrl->cut_start);
    if (rc == 0) rc = _take_u16(c, DLT698_TAG_LONG_UNSIGNED, &ctrl->cut_len);
    if (rc != 0) return rc;
    ctrl->report_and_wait = flag != 0;
    return 0;
}

static int _decode_tp_frames(axdr_cursor_t *c, dcu_transparent_content_t *content) {
    uint8_t count = 0;
    int rc = _take_array(c, &count);
    if (rc != 0) return rc;
    if (count > DCU_MAX_TRANSPARENT_FRAMES) return EDGE_SLAVE_SRC_TEMPORARY;
    content->frame_count = count;
    for (uint8_t i = 0; i < count; i++) {
        rc = _take_head(c, DLT698_TAG_STRUCTURE, 2);
        if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &content->frames[i].seq);
        if (rc == 0)
            rc = _take_octets(c, content->frames[i].data, DCU_MAX_TRANSPARENT_FRAME_LEN,
                              &content->frames[i].len);
        if (rc != 0) return rc;
    }
    return 0;
}

static int _decode_tp_content(axdr_cursor_t *c, dcu_transparent_content_t *out) {
    int rc = _take_head(c, DLT698_TAG_STRUCTURE, 6);
    if (rc == 0) rc = _take_u16(c, DLT698_TAG_LONG_UNSIGNED, &out->seq);
    if (rc == 0) rc = _take_tsa(c, &out->address);
    if (rc == 0) rc = _take_u16(c, DLT698_TAG_LONG_UNSIGNED, &out->pre_script);
    if (rc == 0) rc = _take_u16(c, DLT698_TAG_LONG_UNSIGNED, &out->post_script);
    if (rc == 0) rc = _decode_tp_ctrl(c, &out->ctrl);
    if (rc == 0) rc = _decode_tp_frames(c, out);
    return rc;
}

static int _decode_tp_scheme(axdr_cursor_t *c, dcu_transparent_scheme_t *out) {
    uint8_t count = 0;
    int rc = _take_head(c, DLT698_TAG_STRUCTURE, 3);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &out->scheme_id);
    if (rc == 0) rc = _take_array(c, &count);
    if (rc != 0) return rc;
    if (count > DCU_MAX_TRANSPARENT_CONTENTS) return EDGE_SLAVE_SRC_TEMPORARY;
    out->content_count = count;
    for (uint8_t i = 0; i < count; i++) {
        rc = _decode_tp_content(c, &out->contents[i]);
        if (rc != 0) return rc;
    }
    return _take_u16(c, DLT698_TAG_LONG_UNSIGNED, &out->storage_depth);
}

int dcu_698_decode_transparent_op(uint8_t method, const uint8_t *buf, size_t len,
                                  dcu_transparent_op_t *out) {
    if (!buf || !out || len == 0 || len > UINT16_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    axdr_cursor_t c = {.p = buf, .len = len, .off = 0};
    memset(out, 0, sizeof(*out));
    out->method = method;
    uint8_t count = 0;
    int rc = 0;
    switch (method) {
        case 127:
            rc = _decode_tp_scheme(&c, &out->scheme);
            if (rc != 0) return rc;
            break;
        case 128: {
            uint16_t scheme_id = 0;
            rc = _take_head(&c, DLT698_TAG_STRUCTURE, 4);
            if (rc == 0) rc = _take_u16(&c, DLT698_TAG_LONG_UNSIGNED, &scheme_id);
            if (rc == 0) rc = _take_tsa(&c, &out->content.address);
            if (rc == 0) rc = _decode_tp_ctrl(&c, &out->content.ctrl);
            if (rc == 0) rc = _decode_tp_frames(&c, &out->content);
            if (rc != 0) return rc;
            if (scheme_id == 0 || scheme_id > UINT8_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            out->scheme_id = (uint8_t)scheme_id;
            break;
        }
        case 129: {
            uint8_t id = 0;
            rc = _take_head(&c, DLT698_TAG_STRUCTURE, 2);
            if (rc == 0) rc = _take_u8(&c, DLT698_TAG_UNSIGNED, &id);
            if (rc == 0) rc = _take_array(&c, &count);
            if (rc != 0) return rc;
            if (count == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (count > DCU_MAX_TRANSPARENT_CONTENTS) return EDGE_SLAVE_SRC_TEMPORARY;
            out->scheme_id = id;
            out->addr_count = count;
            for (uint8_t i = 0; i < count; i++) {
                rc = _take_tsa(&c, &out->addresses[i]);
                if (rc != 0) return rc;
            }
            break;
        }
        case 130:
            rc = _take_array(&c, &count);
            if (rc != 0) return rc;
            if (count == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (count > DCU_MAX_TRANSPARENT_SCHEMES) return EDGE_SLAVE_SRC_TEMPORARY;
            for (uint8_t i = 0; i < count; i++) {
                rc = _take_u8(&c, DLT698_TAG_UNSIGNED, &out->ids[i]);
                if (rc != 0) return rc;
            }
            out->id_count = count;
            break;
        case 131: {
            const uint8_t *p = NULL;
            if (!_take(&c, 1, &p) || p[0] != DLT698_TAG_NULL)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            break;
        }
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
    return _time_tag_absent(buf, c.off, c.len) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

static int _put_tsa(uint8_t *buf, size_t cap, size_t *off, const dcu_tsa_t *tsa) {
    if (tsa->len < 2 || tsa->len > DCU_TSA_MAX) return -1;
    if (*off + 1 + tsa->len > cap) return -1;
    buf[(*off)++] = tsa->len;
    memcpy(buf + *off, tsa->bytes, tsa->len);
    *off += tsa->len;
    return 0;
}

static int _put_octets(uint8_t *buf, size_t cap, size_t *off, const uint8_t *data, uint16_t len) {
    if (len > UINT8_MAX) return -1;
    if (*off + 2 + len > cap) return -1;
    buf[(*off)++] = DLT698_TAG_OCTET_STRING;
    buf[(*off)++] = (uint8_t)len;
    if (len > 0) memcpy(buf + *off, data, len);
    *off += len;
    return 0;
}

static int _encode_tp_ctrl(uint8_t *buf, size_t cap, size_t *off,
                           const dcu_transparent_ctrl_t *ctrl) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 4) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_BOOLEAN, ctrl->report_and_wait ? 1 : 0) != 0 ||
        _put_tagged_u16(buf, cap, off, DLT698_TAG_LONG_UNSIGNED, ctrl->wait_timeout_s) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_ENUM, ctrl->compare_mode) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 3) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, ctrl->feature_byte) != 0 ||
        _put_tagged_u16(buf, cap, off, DLT698_TAG_LONG_UNSIGNED, ctrl->cut_start) != 0 ||
        _put_tagged_u16(buf, cap, off, DLT698_TAG_LONG_UNSIGNED, ctrl->cut_len) != 0) {
        return -1;
    }
    return 0;
}

static int _encode_tp_frames(uint8_t *buf, size_t cap, size_t *off,
                             const dcu_transparent_content_t *content) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_ARRAY, content->frame_count) != 0) return -1;
    for (uint8_t i = 0; i < content->frame_count; i++) {
        if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 2) != 0 ||
            _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, content->frames[i].seq) != 0 ||
            _put_octets(buf, cap, off, content->frames[i].data, content->frames[i].len) != 0) {
            return -1;
        }
    }
    return 0;
}

static int _encode_tp_content(uint8_t *buf, size_t cap, size_t *off,
                              const dcu_transparent_content_t *content) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 6) != 0 ||
        _put_tagged_u16(buf, cap, off, DLT698_TAG_LONG_UNSIGNED, content->seq) != 0 ||
        _put_tsa(buf, cap, off, &content->address) != 0 ||
        _put_tagged_u16(buf, cap, off, DLT698_TAG_LONG_UNSIGNED, content->pre_script) != 0 ||
        _put_tagged_u16(buf, cap, off, DLT698_TAG_LONG_UNSIGNED, content->post_script) != 0) {
        return -1;
    }
    if (_encode_tp_ctrl(buf, cap, off, &content->ctrl) != 0) return -1;
    return _encode_tp_frames(buf, cap, off, content);
}

static int _encode_tp_scheme(uint8_t *buf, size_t cap, size_t *off,
                             const dcu_transparent_scheme_t *s) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 3) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, s->scheme_id) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_ARRAY, s->content_count) != 0) {
        return -1;
    }
    for (uint8_t i = 0; i < s->content_count; i++) {
        if (_encode_tp_content(buf, cap, off, &s->contents[i]) != 0) return -1;
    }
    return _put_tagged_u16(buf, cap, off, DLT698_TAG_LONG_UNSIGNED, s->storage_depth);
}

int dcu_698_transparent_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem = (uint8_t)(oad & 0xFF);
    size_t off = 0;
    if (attr_id != 2) {
        return dcu_698_class_defines_attr(IC_COLLECTION, attr_id)
                   ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                   : EDGE_SLAVE_SRC_UNDEFINED;
    }
    if (oi == OI_TRANSPARENT_SCHEME) {
        int count = dcu_scheme_store_transparent_count();
        if (count < 0) return EDGE_SLAVE_SRC_TEMPORARY;
        if (count > 255) return EDGE_SLAVE_SRC_TEMPORARY;
        if (_put_tagged_u8(buf, cap, &off, DLT698_TAG_ARRAY, (uint8_t)count) != 0)
            return EDGE_SLAVE_SRC_TEMPORARY;
        for (int i = 0; i < count; i++) {
            dcu_transparent_scheme_t s;
            int rc = dcu_scheme_store_transparent_at((uint16_t)i, &s);
            if (rc != 0) return rc;
            if (_encode_tp_scheme(buf, cap, &off, &s) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
        }
    } else {
        dcu_transparent_scheme_t s;
        int rc = elem ? dcu_scheme_store_transparent_get(elem, &s)
                      : dcu_scheme_store_transparent_at(0, &s);
        if (rc != 0) return rc;
        if (_encode_tp_scheme(buf, cap, &off, &s) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    }
    *len = off;
    return 0;
}

static int _encode_result(uint8_t *buf, size_t cap, size_t *off,
                          const dcu_transparent_result_t *r) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 4) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, r->scheme_id) != 0) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (dcu_698_put_date_time_s(r->exec_time, buf, cap, off) != 0)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (_put_tsa(buf, cap, off, &r->address) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_ARRAY, r->item_count) != 0)
        return EDGE_SLAVE_SRC_TEMPORARY;
    for (uint8_t i = 0; i < r->item_count; i++) {
        const dcu_transparent_result_item_t *item = &r->items[i];
        if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 3) != 0 ||
            _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, item->seq) != 0) {
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        if (dcu_698_put_date_time_s(item->resp_time, buf, cap, off) != 0)
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        if (_put_octets(buf, cap, off, item->data, item->len) != 0)
            return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return 0;
}

int dcu_698_transparent_result_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem = (uint8_t)(oad & 0xFF);
    size_t off = 0;
    if (attr_id != 2) {
        return dcu_698_class_defines_attr(IC_COLLECTION, attr_id)
                   ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                   : EDGE_SLAVE_SRC_UNDEFINED;
    }
    if (oi == OI_TRANSPARENT_RESULT) {
        int count = dcu_scheme_store_result_count();
        if (count < 0) return EDGE_SLAVE_SRC_TEMPORARY;
        if (count > 255) return EDGE_SLAVE_SRC_TEMPORARY;
        if (_put_tagged_u8(buf, cap, &off, DLT698_TAG_ARRAY, (uint8_t)count) != 0)
            return EDGE_SLAVE_SRC_TEMPORARY;
        for (int i = 0; i < count; i++) {
            dcu_transparent_result_t r;
            int rc = dcu_scheme_store_result_at((uint16_t)i, &r);
            if (rc != 0) return rc;
            int erc = _encode_result(buf, cap, &off, &r);
            if (erc != 0) return erc;
        }
    } else {
        dcu_transparent_result_t r;
        int rc = dcu_scheme_store_result_get(elem, &r);
        if (rc != 0) return rc;
        int erc = _encode_result(buf, cap, &off, &r);
        if (erc != 0) return erc;
    }
    *len = off;
    return 0;
}

static int _take_report_unit(axdr_cursor_t *c, dcu_report_scheme_cfg_t *u) {
    uint8_t timeout_unit = 0;
    uint8_t max_times = 0;
    uint8_t content_type = 0;
    const uint8_t *p = NULL;
    int rc = _take_head(c, DLT698_TAG_STRUCTURE, 5);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &u->scheme_id);
    if (rc == 0) rc = _take_array(c, &u->channel_count);
    if (rc != 0) return rc;
    if (u->channel_count == 0 || u->channel_count > DCU_MAX_REPORT_CHANNELS)
        return EDGE_SLAVE_SRC_TEMPORARY;
    for (uint8_t i = 0; i < u->channel_count; i++) {
        if (!_take(c, 5, &p) || p[0] != DLT698_TAG_OAD) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        u->channels[i] = ((uint32_t)p[1] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 8) |
                         (uint32_t)p[4];
    }
    rc = _take_ti(c, &timeout_unit, &u->timeout_value);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &max_times);
    if (rc == 0) rc = _take_head(c, DLT698_TAG_STRUCTURE, 2);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &content_type);
    if (rc != 0) return rc;
    u->timeout_unit = timeout_unit;
    u->max_times = max_times;
    if (content_type == DCU_REPORT_CONTENT_RECORD) return EDGE_SLAVE_SRC_TEMPORARY;
    if (content_type != DCU_REPORT_CONTENT_OAD) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (!_take(c, 5, &p) || p[0] != DLT698_TAG_OAD) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    u->content_type = DCU_REPORT_CONTENT_OAD;
    u->content_oad =
        ((uint32_t)p[1] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 8) | (uint32_t)p[4];
    return 0;
}

int dcu_698_decode_report_op(uint8_t method, const uint8_t *buf, size_t len, dcu_report_op_t *out) {
    if (!buf || !out || len == 0 || len > UINT16_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    axdr_cursor_t c = {.p = buf, .len = len, .off = 0};
    memset(out, 0, sizeof(*out));
    out->method = method;
    uint8_t count = 0;
    int rc = 0;
    switch (method) {
        case 127:
            rc = _take_array(&c, &count);
            if (rc != 0) return rc;
            if (count == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (count > DCU_MAX_REPORT_SCHEMES) return EDGE_SLAVE_SRC_TEMPORARY;
            for (uint8_t i = 0; i < count; i++) {
                rc = _take_report_unit(&c, &out->units[i]);
                if (rc != 0) return rc;
                for (uint8_t j = 0; j < i; j++) {
                    if (out->units[j].scheme_id == out->units[i].scheme_id)
                        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
                }
            }
            out->unit_count = count;
            break;
        case 128:
            rc = _take_array(&c, &count);
            if (rc != 0) return rc;
            if (count == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (count > DCU_MAX_REPORT_SCHEMES) return EDGE_SLAVE_SRC_TEMPORARY;
            for (uint8_t i = 0; i < count; i++) {
                rc = _take_u8(&c, DLT698_TAG_UNSIGNED, &out->ids[i]);
                if (rc != 0) return rc;
            }
            out->id_count = count;
            break;
        case 129: {
            const uint8_t *p = NULL;
            if (!_take(&c, 1, &p) || p[0] != DLT698_TAG_NULL) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            break;
        }
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
    return _time_tag_absent(buf, c.off, c.len) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

static int _encode_report_unit(uint8_t *buf, size_t cap, size_t *off,
                               const dcu_report_scheme_cfg_t *u) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 5) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, u->scheme_id) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_ARRAY, u->channel_count) != 0) {
        return -1;
    }
    for (uint8_t i = 0; i < u->channel_count; i++) {
        if (_put_raw_u8(buf, cap, off, DLT698_TAG_OAD) != 0 ||
            _put_oad_raw(buf, cap, off, u->channels[i]) != 0) {
            return -1;
        }
    }
    if (_put_raw_u8(buf, cap, off, DLT698_TAG_TI) != 0 ||
        _put_raw_u8(buf, cap, off, u->timeout_unit) != 0 ||
        _put_raw_u8(buf, cap, off, (uint8_t)(u->timeout_value >> 8)) != 0 ||
        _put_raw_u8(buf, cap, off, (uint8_t)u->timeout_value) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, u->max_times) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 2) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, (uint8_t)u->content_type) != 0 ||
        _put_raw_u8(buf, cap, off, DLT698_TAG_OAD) != 0 ||
        _put_oad_raw(buf, cap, off, u->content_oad) != 0) {
        return -1;
    }
    return 0;
}

int dcu_698_report_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem = (uint8_t)(oad & 0xFF);
    size_t off = 0;
    if (attr_id != 2) {
        uint8_t class_id = oi == OI_REPORT_SCHEME ? IC_COLLECTION : IC_PARAM;
        return dcu_698_class_defines_attr(class_id, attr_id) ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                                                             : EDGE_SLAVE_SRC_UNDEFINED;
    }
    if (oi == OI_REPORT_SCHEME) {
        int count = dcu_scheme_store_report_count();
        if (count < 0) return EDGE_SLAVE_SRC_TEMPORARY;
        if (count > 255) return EDGE_SLAVE_SRC_TEMPORARY;
        if (_put_tagged_u8(buf, cap, &off, DLT698_TAG_ARRAY, (uint8_t)count) != 0)
            return EDGE_SLAVE_SRC_TEMPORARY;
        for (int i = 0; i < count; i++) {
            dcu_report_scheme_cfg_t u;
            int rc = dcu_scheme_store_report_at((uint16_t)i, &u);
            if (rc != 0) return rc;
            if (_encode_report_unit(buf, cap, &off, &u) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
        }
    } else {
        dcu_report_scheme_cfg_t u;
        int rc = elem ? dcu_scheme_store_report_get(elem, &u) : dcu_scheme_store_report_at(0, &u);
        if (rc != 0) return rc;
        if (_encode_report_unit(buf, cap, &off, &u) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    }
    *len = off;
    return 0;
}

static int _take_di_array(axdr_cursor_t *c, uint8_t width, uint32_t *di, uint8_t *has) {
    uint8_t count = 0;
    int rc = _take_array(c, &count);
    if (rc != 0) return rc;
    if (count > 1) return EDGE_SLAVE_SRC_TEMPORARY;
    *has = count;
    if (count == 0) return 0;
    const uint8_t *p = NULL;
    if (!_take(c, 2, &p) || p[0] != DLT698_TAG_OCTET_STRING || p[1] != width)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (!_take(c, width, &p)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint32_t v = 0;
    for (uint8_t i = 0; i < width; i++) v |= (uint32_t)p[i] << (8 * i);
    *di = v;
    return 0;
}

static int _take_rule_csd(axdr_cursor_t *c, uint32_t *oad) {
    const uint8_t *p = NULL;
    if (!_take(c, 2, &p) || p[0] != DCU_698_TAG_CSD) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (p[1] == DLT698_CSD_ROAD) return EDGE_SLAVE_SRC_TEMPORARY;
    if (p[1] != DLT698_CSD_OAD) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (!_take(c, 4, &p)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    *oad = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
    return 0;
}

static int _take_rule_unit(axdr_cursor_t *c, dcu_rule_unit_t *u) {
    memset(u, 0, sizeof(*u));
    int rc = _take_head(c, DLT698_TAG_STRUCTURE, 2);
    if (rc == 0) rc = _take_rule_csd(c, &u->csd);
    if (rc == 0) rc = _take_head(c, DLT698_TAG_STRUCTURE, 3);
    if (rc == 0) rc = _take_head(c, DLT698_TAG_STRUCTURE, 2);
    if (rc == 0) rc = _take_di_array(c, 4, &u->di_2007_main, &u->has_2007_main);
    if (rc == 0) rc = _take_di_array(c, 4, &u->di_2007_spare, &u->has_2007_spare);
    if (rc == 0) rc = _take_head(c, DLT698_TAG_STRUCTURE, 2);
    uint32_t di16 = 0;
    if (rc == 0) rc = _take_di_array(c, 2, &di16, &u->has_1997_main);
    if (rc == 0) u->di_1997_main = (uint16_t)di16;
    if (rc == 0) rc = _take_di_array(c, 2, &di16, &u->has_1997_spare);
    if (rc == 0) u->di_1997_spare = (uint16_t)di16;
    if (rc == 0) rc = _take_head(c, DLT698_TAG_STRUCTURE, 1);
    if (rc != 0) return rc;
    const uint8_t *p = NULL;
    if (!_take(c, 2, &p) || p[0] != DLT698_TAG_OCTET_STRING) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint8_t frame_len = p[1];
    if (!_take(c, frame_len, &p)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    u->frame_len = frame_len;
    if (frame_len > 0) memcpy(u->frame, p, frame_len);
    return 0;
}

int dcu_698_decode_rule_op(uint8_t method, const uint8_t *buf, size_t len, dcu_rule_op_t *out) {
    if (!buf || !out || len == 0 || len > UINT16_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    axdr_cursor_t c = {.p = buf, .len = len, .off = 0};
    memset(out, 0, sizeof(*out));
    out->method = method;
    uint8_t count = 0;
    int rc = 0;
    switch (method) {
        case 127:
            rc = _take_array(&c, &count);
            if (rc != 0) return rc;
            if (count == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (count > DCU_RULE_BATCH_MAX) return EDGE_SLAVE_SRC_TEMPORARY;
            for (uint8_t i = 0; i < count; i++) {
                rc = _take_rule_unit(&c, &out->units[i]);
                if (rc != 0) return rc;
            }
            out->unit_count = count;
            break;
        case 128:
            rc = _take_array(&c, &count);
            if (rc != 0) return rc;
            if (count == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (count > DCU_RULE_BATCH_MAX) return EDGE_SLAVE_SRC_TEMPORARY;
            for (uint8_t i = 0; i < count; i++) {
                rc = _take_rule_csd(&c, &out->csds[i]);
                if (rc != 0) return rc;
            }
            out->csd_count = count;
            break;
        case 129: {
            const uint8_t *p = NULL;
            if (!_take(&c, 1, &p) || p[0] != DLT698_TAG_NULL) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            break;
        }
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
    return _time_tag_absent(buf, c.off, c.len) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

static int _put_di_array(uint8_t *buf, size_t cap, size_t *off, uint8_t width, uint32_t di,
                         uint8_t has) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_ARRAY, has ? 1 : 0) != 0) return -1;
    if (!has) return 0;
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_OCTET_STRING, width) != 0) return -1;
    for (uint8_t i = 0; i < width; i++) {
        if (_put_raw_u8(buf, cap, off, (uint8_t)(di >> (8 * i))) != 0) return -1;
    }
    return 0;
}

static int _encode_rule_unit(uint8_t *buf, size_t cap, size_t *off, const dcu_rule_unit_t *u) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 2) != 0 ||
        _put_raw_u8(buf, cap, off, DCU_698_TAG_CSD) != 0 ||
        _put_raw_u8(buf, cap, off, DLT698_CSD_OAD) != 0 ||
        _put_oad_raw(buf, cap, off, u->csd) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 3) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 2) != 0 ||
        _put_di_array(buf, cap, off, 4, u->di_2007_main, u->has_2007_main) != 0 ||
        _put_di_array(buf, cap, off, 4, u->di_2007_spare, u->has_2007_spare) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 2) != 0 ||
        _put_di_array(buf, cap, off, 2, u->di_1997_main, u->has_1997_main) != 0 ||
        _put_di_array(buf, cap, off, 2, u->di_1997_spare, u->has_1997_spare) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 1) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_OCTET_STRING, u->frame_len) != 0) {
        return -1;
    }
    for (uint8_t i = 0; i < u->frame_len; i++) {
        if (_put_raw_u8(buf, cap, off, u->frame[i]) != 0) return -1;
    }
    return 0;
}

int dcu_698_rule_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem = (uint8_t)(oad & 0xFF);
    size_t off = 0;
    if (attr_id != 2) {
        uint8_t class_id = oi == OI_RULE_LIBRARY ? IC_COLLECTION : IC_PARAM;
        return dcu_698_class_defines_attr(class_id, attr_id) ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                                                             : EDGE_SLAVE_SRC_UNDEFINED;
    }
    if (oi == OI_RULE_LIBRARY) {
        uint16_t count = 0;
        if (dcu_698_rules_projected_count(&count) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
        if (_put_tagged_u8(buf, cap, &off, DLT698_TAG_ARRAY, (uint8_t)count) != 0)
            return EDGE_SLAVE_SRC_TEMPORARY;
        for (uint16_t i = 0; i < count; i++) {
            uint32_t rule_oad = 0;
            dcu_rule_unit_t u;
            if (dcu_698_rules_projected_oad(i, &rule_oad) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
            if (dcu_698_rules_unit_load(rule_oad, &u) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
            if (_encode_rule_unit(buf, cap, &off, &u) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
        }
    } else {
        uint16_t count = 0;
        if (dcu_698_rules_projected_count(&count) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
        if (elem >= count) return EDGE_SLAVE_SRC_NOT_FOUND;
        uint32_t rule_oad = 0;
        if (dcu_698_rules_projected_oad(elem, &rule_oad) != 0) return EDGE_SLAVE_SRC_NOT_FOUND;
        dcu_rule_unit_t u;
        int rc = dcu_698_rules_unit_load(rule_oad, &u);
        if (rc != 0) return rc == -1 ? EDGE_SLAVE_SRC_NOT_FOUND : EDGE_SLAVE_SRC_TEMPORARY;
        if (_encode_rule_unit(buf, cap, &off, &u) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    }
    *len = off;
    return 0;
}

/* ==================== 6034/6035 采集任务监控 ==================== */

static int _put_long_unsigned(uint8_t *buf, size_t cap, size_t *off, uint32_t v) {
    if (v > UINT16_MAX) return -1;
    if (*off + 3 > cap) return -1;
    buf[(*off)++] = DLT698_TAG_LONG_UNSIGNED;
    buf[(*off)++] = (uint8_t)(v >> 8);
    buf[(*off)++] = (uint8_t)v;
    return 0;
}

/* 未执行/执行中缺一侧时标：按计划 §11.1.10 用 year=FFFF、其余=FF 的非法值表示“无”。 */
static int _put_absent_time(uint8_t *buf, size_t cap, size_t *off) {
    if (*off + 8 > cap) return -1;
    buf[(*off)++] = DLT698_TAG_DATE_TIME_S;
    for (int i = 0; i < 7; i++) buf[(*off)++] = 0xFF;
    return 0;
}

static int _put_epoch_or_absent(uint8_t *buf, size_t cap, size_t *off, uint64_t epoch_s) {
    if (epoch_s == 0) return _put_absent_time(buf, cap, off);
    return dcu_698_put_date_time_s(epoch_s, buf, cap, off);
}

static int _encode_task_monitor_unit(uint8_t *buf, size_t cap, size_t *off,
                                     const dcu_task_monitor_t *m) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 8) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, (uint8_t)m->task_id) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_ENUM, m->status) != 0) {
        return -1;
    }
    if (_put_epoch_or_absent(buf, cap, off, m->start_epoch_s) != 0) return -1;
    if (_put_epoch_or_absent(buf, cap, off, m->end_epoch_s) != 0) return -1;
    if (_put_long_unsigned(buf, cap, off, m->total_meters) != 0) return -1;
    if (_put_long_unsigned(buf, cap, off, m->success_meters) != 0) return -1;
    if (_put_long_unsigned(buf, cap, off, m->sent_packets) != 0) return -1;
    if (_put_long_unsigned(buf, cap, off, m->recv_packets) != 0) return -1;
    return 0;
}

int dcu_698_task_monitor_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem = (uint8_t)(oad & 0xFF);
    size_t off = 0;
    if (attr_id != 2) {
        uint8_t class_id = oi == OI_TASK_MONITOR_SET ? IC_COLLECTION : IC_PARAM;
        return dcu_698_class_defines_attr(class_id, attr_id) ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                                                             : EDGE_SLAVE_SRC_UNDEFINED;
    }
    const dcu_698_object_t *task_obj = dcu_698_find(OI_TASK_SCHEDULER);
    dcu_poller_t *poller = task_obj ? (dcu_poller_t *)task_obj->instance : NULL;
    if (!poller) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (oi == OI_TASK_MONITOR_SET) {
        int count = dcu_poller_task_monitor_count(poller);
        if (count < 0 || count > 255) return EDGE_SLAVE_SRC_TEMPORARY;
        if (_put_tagged_u8(buf, cap, &off, DLT698_TAG_ARRAY, (uint8_t)count) != 0)
            return EDGE_SLAVE_SRC_TEMPORARY;
        for (int i = 0; i < count; i++) {
            dcu_task_monitor_t m;
            if (dcu_poller_task_monitor_at(poller, i, &m) != 0)
                return EDGE_SLAVE_SRC_TEMPORARY;
            if (_encode_task_monitor_unit(buf, cap, &off, &m) != 0)
                return EDGE_SLAVE_SRC_TEMPORARY;
        }
    } else {
        dcu_task_monitor_t m;
        if (dcu_poller_task_monitor_get(poller, elem, &m) != 0) return EDGE_SLAVE_SRC_NOT_FOUND;
        if (_encode_task_monitor_unit(buf, cap, &off, &m) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    }
    *len = off;
    return 0;
}
