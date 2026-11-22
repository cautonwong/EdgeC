#include "dcu_698_record.h"

#include <string.h>

#include "dcu_698_axdr.h"
#include "dcu_router.h"
#include "dlt698_codec.h"

#define OAD_FREEZE_TIME 0x20210200U
#define OI_DAILY_FREEZE 0x5004U

typedef struct {
    uint32_t oad;
    uint8_t tag;
    int8_t scaler;
} record_col_t;

static const record_col_t k_cols[] = {
    {OAD_ACTIVE_ENERGY, 0x06, -2},
    {OAD_REACTIVE_ENERGY, 0x06, -2},
    {OAD_VOLTAGE, 0x12, -1},
    {OAD_CURRENT, 0x05, -3},
    {OAD_ACTIVE_POWER_TOTAL, 0x05, -1},
    {OAD_POWER_FACTOR_TOTAL, 0x10, -3},
    {OAD_GRID_FREQUENCY, 0x12, -2},
};

static const record_col_t *_find_col(uint32_t oad) {
    for (size_t i = 0; i < sizeof(k_cols) / sizeof(k_cols[0]); i++) {
        if (k_cols[i].oad == oad) return &k_cols[i];
    }
    return NULL;
}

static int64_t _scale_to_raw(double value, int8_t scaler) {
    double factor = 1.0;
    for (int i = 0; i < -scaler; i++) factor *= 10.0;
    for (int i = 0; i < scaler; i++) factor /= 10.0;
    double x = value * factor;
    return (int64_t)(x >= 0 ? x + 0.5 : x - 0.5);
}

static int _put_int(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, int64_t raw) {
    size_t size = 0;
    switch (tag) {
        case 0x12:
            if (raw < 0 || raw > 0xFFFF) return -1;
            size = 2;
            break;
        case 0x10:
            if (raw < -32768 || raw > 32767) return -1;
            size = 2;
            break;
        case 0x05:
            if (raw < INT32_MIN || raw > INT32_MAX) return -1;
            size = 4;
            break;
        case 0x06:
            if (raw < 0 || (uint64_t)raw > UINT32_MAX) return -1;
            size = 4;
            break;
        default:
            return -1;
    }
    if (*off + 1 + size > cap) return -1;
    buf[(*off)++] = tag;
    uint64_t x = (uint64_t)raw;
    for (size_t i = 0; i < size; i++) {
        buf[*off + size - 1 - i] = (uint8_t)(x >> (8 * i));
    }
    *off += size;
    return 0;
}

void dcu_698_record_init(dcu_698_record_ctx_t *ctx, dcu_storage_t *storage,
                         dcu_inventory_t *inventory) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->storage = storage;
    ctx->inventory = inventory;
}

static size_t _distinct_times(const dcu_freeze_record_t *page, size_t n, uint64_t *times,
                              size_t max) {
    size_t count = 0;
    for (size_t i = 0; i < n; i++) {
        if (count == 0 || page[i].freeze_time != times[count - 1]) {
            if (count >= max) return max + 1;
            times[count++] = page[i].freeze_time;
        }
    }
    return count;
}

int dcu_698_record_begin(void *opaque, uint32_t oad, const uint8_t *selection, size_t selection_len,
                         size_t *row_count) {
    dcu_698_record_ctx_t *ctx = (dcu_698_record_ctx_t *)opaque;
    if (!ctx || !ctx->storage || !ctx->inventory || !row_count) {
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    *row_count = 0;
    ctx->row_count = 0;
    ctx->page_count = 0;

    if ((oad >> 16) != OI_DAILY_FREEZE || ((oad >> 8) & 0xFF) != 2) {
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    uint8_t slot = (uint8_t)(oad & 0xFF);
    const production_meter_t *meter = dcu_inventory_by_slot(ctx->inventory, slot);
    if (!meter) return EDGE_SLAVE_SRC_NOT_FOUND;

    /* 端口只给 RSD+RCSD；codec 要 OAD 开头，按固定上界重建。 */
    uint8_t req[4 + 128];
    if (selection_len > sizeof(req) - 4) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    req[0] = (uint8_t)(oad >> 24);
    req[1] = (uint8_t)(oad >> 16);
    req[2] = (uint8_t)(oad >> 8);
    req[3] = (uint8_t)oad;
    if (selection_len > 0) memcpy(req + 4, selection, selection_len);

    uint32_t parsed_oad = 0;
    dlt698_record_sel_t sel;
    dlt698_rcsd_t rcsd;
    size_t used = 0;
    int rc =
        dlt698_decode_get_record_request(req, 4 + selection_len, &parsed_oad, &sel, &rcsd, &used);
    if (rc == DLT698_RECORD_UNSUPPORTED_SELECTOR || rc == DLT698_RECORD_UNSUPPORTED_DATA) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (rc != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (parsed_oad != oad || rcsd.count > DCU_RECORD_MAX_COLS) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    for (uint8_t i = 0; i < rcsd.count; i++) {
        if (rcsd.items[i].kind != DLT698_CSD_OAD) return EDGE_SLAVE_SRC_TEMPORARY;
        ctx->col_oad[i] = rcsd.items[i].oad;
    }
    ctx->cols[0] = rcsd.count;

    uint64_t start = 0, end = (uint64_t)INT64_MAX;
    if (sel.selector == DLT698_RSD_SELECTOR1) {
        if (sel.oad != OAD_FREEZE_TIME) return EDGE_SLAVE_SRC_TEMPORARY;
        if (dcu_698_date_time_s_to_epoch(sel.value, sel.value_len, &start) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        end = start;
    } else if (sel.selector == DLT698_RSD_SELECTOR2) {
        if (sel.oad != OAD_FREEZE_TIME) return EDGE_SLAVE_SRC_TEMPORARY;
        /* 区间是前闭后开；存储查询用闭区间。 */
        if (sel.interval_len > 1 || (sel.interval_len == 1 && sel.interval[0] != 0x00)) {
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        if (dcu_698_date_time_s_to_epoch(sel.start, sel.start_len, &start) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        if (dcu_698_date_time_s_to_epoch(sel.end, sel.end_len, &end) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        if (end != (uint64_t)INT64_MAX && end > 0) end -= 1;
    } else if (sel.selector != DLT698_RSD_NO_SELECTION) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }

    uint16_t count = 0;
    if (dcu_storage_query_freeze(ctx->storage, meter->address, OAD_DAILY_FREEZE, start, end,
                                 ctx->page, DCU_RECORD_PAGE_ROWS, &count) != 0) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (count > DCU_RECORD_PAGE_CAP) return EDGE_SLAVE_SRC_TEMPORARY;

    /* 表31：RCSD 个数 0 = 全部列。记录里可用的列 = 冻结时间 + 本页出现的源 OAD。 */
    if (ctx->cols[0] == 0) {
        ctx->col_oad[0] = OAD_FREEZE_TIME;
        uint8_t c = 1;
        for (uint16_t i = 0; i < count && c < DCU_RECORD_MAX_COLS; i++) {
            uint32_t src = ctx->page[i].source_oad;
            bool seen = false;
            for (uint8_t j = 1; j < c; j++) {
                if (ctx->col_oad[j] == src) seen = true;
            }
            if (!seen) ctx->col_oad[c++] = src;
        }
        ctx->cols[0] = c;
    }

    uint64_t times[DCU_RECORD_MAX_ROWS];
    size_t rows = _distinct_times(ctx->page, count, times, DCU_RECORD_MAX_ROWS);
    if (rows > DCU_RECORD_MAX_ROWS) return EDGE_SLAVE_SRC_TEMPORARY;

    ctx->page_count = count;
    ctx->row_count = rows;
    ctx->oad = oad;
    *row_count = rows;
    return 0;
}

static const dcu_freeze_record_t *_find_cell(const dcu_698_record_ctx_t *ctx, uint64_t time,
                                             uint32_t source_oad) {
    for (size_t i = 0; i < ctx->page_count; i++) {
        if (ctx->page[i].freeze_time == time && ctx->page[i].source_oad == source_oad) {
            return &ctx->page[i];
        }
    }
    return NULL;
}

int dcu_698_record_row(void *opaque, uint32_t oad, size_t index, uint8_t *row, size_t cap,
                       size_t *row_len) {
    dcu_698_record_ctx_t *ctx = (dcu_698_record_ctx_t *)opaque;
    if (!ctx || !row || !row_len || oad != ctx->oad || index >= ctx->row_count) {
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    *row_len = 0;

    size_t seen = 0;
    uint64_t time = 0;
    int found = 0;
    for (size_t i = 0; i < ctx->page_count; i++) {
        if (i == 0 || ctx->page[i].freeze_time != ctx->page[i - 1].freeze_time) {
            if (seen == index) {
                time = ctx->page[i].freeze_time;
                found = 1;
                break;
            }
            seen++;
        }
    }
    if (!found) return EDGE_SLAVE_SRC_NOT_FOUND;

    size_t off = 0;
    for (uint8_t c = 0; c < ctx->cols[0]; c++) {
        uint32_t col = ctx->col_oad[c];
        if (col == OAD_FREEZE_TIME) {
            if (dcu_698_put_date_time_s(time, row, cap, &off) != 0)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            continue;
        }
        const record_col_t *desc = _find_col(col);
        const dcu_freeze_record_t *cell = _find_cell(ctx, time, col);
        if (!desc || !cell) return EDGE_SLAVE_SRC_NOT_FOUND;
        int64_t raw = _scale_to_raw(cell->frozen_value, desc->scaler);
        if (_put_int(row, cap, &off, desc->tag, raw) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
    }
    *row_len = off;
    return 0;
}

int dcu_698_record_end(void *opaque, uint32_t oad) {
    dcu_698_record_ctx_t *ctx = (dcu_698_record_ctx_t *)opaque;
    if (!ctx) return 0;
    (void)oad;
    ctx->row_count = 0;
    ctx->page_count = 0;
    return 0;
}
