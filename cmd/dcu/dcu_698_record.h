#ifndef DCU_698_RECORD_H
#define DCU_698_RECORD_H

#include <stddef.h>
#include <stdint.h>

#include "dcu_inventory.h"
#include "dcu_storage.h"
#include "edge_slave_plane.h"

/* 一页最多 255 条时间（单帧一字节计数，ADR-0014 §2）；查询多取 1 行判截断。 */
#define DCU_RECORD_MAX_ROWS 255
#define DCU_RECORD_PAGE_CAP (DCU_RECORD_MAX_ROWS * 4)
#define DCU_RECORD_PAGE_ROWS (DCU_RECORD_PAGE_CAP + 1)
#define DCU_RECORD_MAX_COLS 8

typedef struct {
    dcu_storage_t *storage;
    dcu_inventory_t *inventory;
    uint32_t oad;
    uint8_t cols[DCU_RECORD_MAX_COLS];
    uint32_t col_oad[DCU_RECORD_MAX_COLS];
    size_t row_count;
    size_t page_count;
    dcu_freeze_record_t page[DCU_RECORD_PAGE_ROWS];
} dcu_698_record_ctx_t;

void dcu_698_record_init(dcu_698_record_ctx_t *ctx, dcu_storage_t *storage,
                         dcu_inventory_t *inventory);

int dcu_698_record_begin(void *ctx, uint32_t oad, const uint8_t *selection, size_t selection_len,
                         size_t *row_count);
int dcu_698_record_row(void *ctx, uint32_t oad, size_t index, uint8_t *row, size_t cap,
                       size_t *row_len);
int dcu_698_record_end(void *ctx, uint32_t oad);

#endif /* DCU_698_RECORD_H */
