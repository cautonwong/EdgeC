#include <cJSON.h>
#include <driver_api.h>
#include <errno.h>
#include <modbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==========================================
// 1. Data Structures
// ==========================================

typedef struct {
  uint32_t tag_id;
  int addr;
  int type;  // 0=UINT16, 1=INT16, 2=FLOAT32_ABCD, 3=FLOAT32_CDAB, etc.
  float scale;
  int offset_in_blk;
} TagInfo;

typedef struct {
  int start_addr;
  int length;
  int tag_count;
  TagInfo **tags;
} ReadBlock;

// This is the concrete implementation of the opaque DriverContext
struct DriverInternalContext {
  modbus_t *mb;
  int is_connected;
  char ip[32];
  int port;
  int slave_id;

  int tag_count;
  TagInfo *tag_list;

  int block_count;
  ReadBlock *blocks;

  OnDataCb on_data;
  void *usr_data;
};

// ==========================================
// 2. Optimizer Algorithm
// ==========================================
static int cmp_tags_by_addr(const void *a, const void *b) {
  return ((TagInfo *)a)->addr - ((TagInfo *)b)->addr;
}

#define MAX_READ_GAP 5
#define MAX_PDU_REGS 120

static void build_optimized_read_blocks(DriverContext *ctx) {
  if (ctx->tag_count == 0) return;

  qsort(ctx->tag_list, ctx->tag_count, sizeof(TagInfo), cmp_tags_by_addr);

  ctx->blocks = calloc(ctx->tag_count, sizeof(ReadBlock));
  ctx->block_count = 0;
  int current_blk_idx = -1;

  for (int i = 0; i < ctx->tag_count; i++) {
    TagInfo *t = &ctx->tag_list[i];
    int reg_len = (t->type >= 2) ? 2 : 1;
    int needs_new_block = 1;

    if (current_blk_idx >= 0) {
      ReadBlock *blk = &ctx->blocks[current_blk_idx];
      int end_addr = blk->start_addr + blk->length;
      int gap = t->addr - end_addr;

      if (gap >= 0 && gap <= MAX_READ_GAP &&
          (t->addr + reg_len - blk->start_addr) <= MAX_PDU_REGS) {
        needs_new_block = 0;
        blk->length = (t->addr + reg_len) - blk->start_addr;
        blk->tag_count++;
        blk->tags = realloc(blk->tags, blk->tag_count * sizeof(TagInfo *));
        blk->tags[blk->tag_count - 1] = t;
        t->offset_in_blk = t->addr - blk->start_addr;
      }
    }

    if (needs_new_block) {
      current_blk_idx++;
      ReadBlock *blk = &ctx->blocks[current_blk_idx];
      ctx->block_count++;
      blk->start_addr = t->addr;
      blk->length = reg_len;
      blk->tag_count = 1;
      blk->tags = malloc(sizeof(TagInfo *));
      blk->tags[0] = t;
      t->offset_in_blk = 0;
    }
  }
  printf("[Modbus] Optimized %d tags into %d read blocks.\n", ctx->tag_count,
         ctx->block_count);
}

// ==========================================
// 3. Data Decoder
// ==========================================
static float extract_float_abcd(uint16_t *buf) {
  uint32_t raw = ((uint32_t)buf[0] << 16) | buf[1];
  float f;
  memcpy(&f, &raw, 4);
  return f;
}

static void decode_and_submit(DriverContext *ctx, ReadBlock *blk,
                              uint16_t *raw_buf) {
  for (int i = 0; i < blk->tag_count; i++) {
    TagInfo *t = blk->tags[i];
    uint16_t *ptr = &raw_buf[t->offset_in_blk];

    cedgex_datapoint_t dp;
    dp.ts = current_ts_ms();
    dp.tag_id = t->tag_id;
    dp.quality = CEDGEX_QUALITY_GOOD;

    if (t->type == 2) {  // FLOAT32_ABCD
      dp.type = CEDGEX_VAL_TYPE_FLOAT;
      dp.val.f_val = extract_float_abcd(ptr) * t->scale;
    } else {  // UINT16
      dp.type = CEDGEX_VAL_TYPE_INT;
      dp.val.i_val = (int64_t)(ptr[0] * t->scale);
    }

    if (ctx->on_data) {
      ctx->on_data(dp.tag_id, &dp, ctx->usr_data);
    }
  }
}

// ==========================================
// 4. DriverOps Implementation
// ==========================================

static DriverContext *modbus_init(const char *json_config, OnDataCb cb,
                                  void *usr_data) {
  DriverContext *ctx = calloc(1, sizeof(DriverContext));
  ctx->on_data = cb;
  ctx->usr_data = usr_data;

  cJSON *root = cJSON_Parse(json_config);
  if (!root) {
    free(ctx);
    return NULL;
  }

  cJSON *conn = cJSON_GetObjectItem(root, "connection");
  strncpy(ctx->ip, cJSON_GetObjectItem(conn, "ip")->valuestring, 31);
  ctx->port = cJSON_GetObjectItem(conn, "port")->valueint;
  ctx->slave_id = cJSON_GetObjectItem(conn, "slave_id")->valueint;

  cJSON *tags = cJSON_GetObjectItem(root, "tags");
  ctx->tag_count = cJSON_GetArraySize(tags);
  ctx->tag_list = calloc(ctx->tag_count, sizeof(TagInfo));

  for (int i = 0; i < ctx->tag_count; i++) {
    cJSON *item = cJSON_GetArrayItem(tags, i);
    TagInfo *t = &ctx->tag_list[i];
    t->tag_id = cJSON_GetObjectItem(item, "tag_id")->valueint;
    t->addr = cJSON_GetObjectItem(item, "addr")->valueint;
    char *type_str = cJSON_GetObjectItem(item, "type")->valuestring;
    if (strcmp(type_str, "float") == 0)
      t->type = 2;
    else
      t->type = 0;
    cJSON *scale_item = cJSON_GetObjectItem(item, "scale");
    t->scale = scale_item ? (float)scale_item->valuedouble : 1.0f;
  }

  build_optimized_read_blocks(ctx);
  cJSON_Delete(root);
  return ctx;
}

static int modbus_start(DriverContext *d) {
  DriverContext *ctx = d;
  ctx->mb = modbus_new_tcp(ctx->ip, ctx->port);
  if (!ctx->mb) {
    fprintf(stderr, "[Modbus] Failed to create context.\n");
    return -1;
  }
  modbus_set_slave(ctx->mb, ctx->slave_id);
  modbus_set_response_timeout(ctx->mb, 0, 500000);  // 500ms

  if (modbus_connect(ctx->mb) == -1) {
    fprintf(stderr, "[Modbus] Conn failed to %s:%d : %s\n", ctx->ip, ctx->port,
            modbus_strerror(errno));
    modbus_free(ctx->mb);
    ctx->mb = NULL;
    ctx->is_connected = 0;
    return -1;
  }
  ctx->is_connected = 1;
  printf("[Modbus] Connected to %s:%d\n", ctx->ip, ctx->port);
  return 0;
}

static int modbus_poll(DriverContext *d) {
  DriverContext *ctx = d;
  if (!ctx->is_connected) {
    if (modbus_connect(ctx->mb) == -1) return -1;
    ctx->is_connected = 1;
  }

  uint16_t *read_buf = malloc(MAX_PDU_REGS * sizeof(uint16_t));
  if (!read_buf) return -1;

  for (int i = 0; i < ctx->block_count; i++) {
    ReadBlock *blk = &ctx->blocks[i];
    int rc =
        modbus_read_registers(ctx->mb, blk->start_addr, blk->length, read_buf);
    if (rc == -1) {
      fprintf(stderr, "[Modbus] Read Error at addr %d: %s\n", blk->start_addr,
              modbus_strerror(errno));
      continue;
    }
    decode_and_submit(ctx, blk, read_buf);
  }

  free(read_buf);
  return 0;
}

static int modbus_stop(DriverContext *d) {
  DriverContext *ctx = d;
  if (ctx->mb) {
    modbus_close(ctx->mb);
    modbus_free(ctx->mb);
    ctx->mb = NULL;
    ctx->is_connected = 0;
  }
  return 0;
}

static void modbus_destroy(DriverContext *d) {
  if (!d) return;
  modbus_stop(d);
  for (int i = 0; i < d->block_count; i++) {
    free(d->blocks[i].tags);
  }
  free(d->blocks);
  free(d->tag_list);
  free(d);
}

// Export the driver operations struct
__attribute__((visibility("default"))) DriverOps *driver_entry_point(void) {
  static DriverOps ops = {.name = "modbus-tcp",
                          .init = modbus_init,
                          .start = modbus_start,
                          .stop = modbus_stop,
                          .poll = modbus_poll,
                          .write_tag = NULL,  // Not implemented
                          .destroy = modbus_destroy};
  return &ops;
}
