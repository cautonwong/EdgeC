/**
 * @file test_iems_bmser_ipc.c
 * @brief P3-01b BMSER-BIN IPC contract: real-file counts, field semantics,
 *        and the strict boundary the loader enforces.
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iems_bmser_ipc.h"
#include "iems_config_loader.h"

#ifndef IEMS_BMSER_IPC_FIXTURE_DIR
#error "IEMS_BMSER_IPC_FIXTURE_DIR must point at tests/fixtures"
#endif

/* ------------------------------------------------------------------ */
/* In-memory source: exercises the same read_file boundary            */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *json;
} mem_source_t;

static iems_config_source_rc_t _mem_read(void *ctx, const char *relative_path, char **data,
                                         size_t *len, char *err, size_t err_size) {
    (void)relative_path;
    (void)err;
    (void)err_size;
    const mem_source_t *src = ctx;
    *data = NULL;
    *len = 0;
    if (!src || !src->json) return IEMS_CONFIG_SOURCE_ABSENT;
    size_t n = strlen(src->json);
    char *copy = malloc(n + 1);
    if (!copy) return IEMS_CONFIG_SOURCE_ERROR;
    memcpy(copy, src->json, n + 1);
    *data = copy;
    *len = n;
    return IEMS_CONFIG_SOURCE_OK;
}

static int _load_mem(const char *json, iems_bmser_ipc_t *model, char *err, size_t err_size) {
    mem_source_t ctx = {json};
    iems_config_source_t source = {_mem_read, NULL, &ctx};
    return iems_bmser_ipc_load(&source, NULL, model, err, err_size);
}

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

static size_t _count_type(const iems_bmser_ipc_t *model, iems_bmser_type_t type) {
    size_t count = 0;
    for (size_t i = 0; i < model->dev_count; i++) {
        for (size_t j = 0; j < model->devs[i].block_count; j++) {
            const iems_bmser_block_t *block = &model->devs[i].blocks[j];
            for (size_t k = 0; k < block->sub_count; k++) {
                if (block->subs[k].type == type) count++;
            }
        }
    }
    return count;
}

static size_t _count_cycle(const iems_bmser_ipc_t *model, uint32_t cycle) {
    size_t count = 0;
    for (size_t i = 0; i < model->dev_count; i++) {
        for (size_t j = 0; j < model->devs[i].block_count; j++) {
            if (model->devs[i].blocks[j].cycle_ms == cycle) count++;
        }
    }
    return count;
}

static size_t _count_scaled(const iems_bmser_ipc_t *model, double scale) {
    size_t count = 0;
    for (size_t i = 0; i < model->dev_count; i++) {
        for (size_t j = 0; j < model->devs[i].block_count; j++) {
            const iems_bmser_block_t *block = &model->devs[i].blocks[j];
            for (size_t k = 0; k < block->sub_count; k++) {
                if (block->subs[k].scale == scale) count++;
            }
        }
    }
    return count;
}

static size_t _count_tag(const iems_bmser_ipc_t *model, const char *dev_id, const char *tag) {
    size_t count = 0;
    for (size_t i = 0; i < model->dev_count; i++) {
        if (strcmp(model->devs[i].dev_id, dev_id) != 0) continue;
        for (size_t j = 0; j < model->devs[i].block_count; j++) {
            const iems_bmser_block_t *block = &model->devs[i].blocks[j];
            for (size_t k = 0; k < block->sub_count; k++) {
                if (strcmp(block->subs[k].tag, tag) == 0) count++;
            }
        }
    }
    return count;
}

static size_t _count_nodesc(const iems_bmser_ipc_t *model) {
    size_t count = 0;
    for (size_t i = 0; i < model->dev_count; i++) {
        for (size_t j = 0; j < model->devs[i].block_count; j++) {
            const iems_bmser_block_t *block = &model->devs[i].blocks[j];
            for (size_t k = 0; k < block->sub_count; k++) {
                if (block->subs[k].name[0] == '\0') count++;
            }
        }
    }
    return count;
}

#define SUB_BASE "{\"type\":\"U16\",\"tag\":\"x\",\"scale\":1}"
#define BLOCK_BASE "{\"block_id\":\"0x1\",\"cycle\":1000,\"sub_id\":[" SUB_BASE "]}"
#define DEV_BASE "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":[" BLOCK_BASE "]}"

static void _expect_reject_doc(const char *doc, const char *needle) {
    iems_bmser_ipc_t model = {0};
    char err[256];
    assert_int_equal(_load_mem(doc, &model, err, sizeof(err)), -1);
    if (!strstr(err, needle)) fprintf(stderr, "doc=%s\nerr=%s\nneedle=%s\n", doc, err, needle);
    assert_true(strstr(err, needle) != NULL);
    assert_null(model.devs);
    assert_int_equal(model.dev_count, 0);
    assert_false(model.loaded);
    iems_bmser_ipc_free(&model);
}

static void _expect_reject_dev(const char *dev_json, const char *needle) {
    char doc[2048];
    int n = snprintf(doc, sizeof(doc), "{\"dev\":[%s]}", dev_json);
    assert_true(n > 0 && (size_t)n < sizeof(doc));
    _expect_reject_doc(doc, needle);
}

/* ------------------------------------------------------------------ */
/* Real file                                                          */
/* ------------------------------------------------------------------ */

static void test_real_file_counts(void **state) {
    (void)state;
    iems_config_file_source_t fs;
    iems_config_file_source_init(&fs, IEMS_BMSER_IPC_FIXTURE_DIR);
    iems_config_source_t source = iems_config_file_source_port(&fs);
    iems_bmser_ipc_t model = {0};
    char err[256];

    assert_int_equal(iems_bmser_ipc_load(&source, IEMS_BMSER_IPC_FILE, &model, err, sizeof(err)),
                     0);
    assert_true(model.loaded);
    assert_int_equal(model.dev_count, 2);
    assert_int_equal(model.block_count, 9);
    assert_int_equal(model.sub_count, 170);
    assert_int_equal(model.placeholder_count, 25);

    /* Type and scale distribution pins the whole file, not one sample. */
    assert_int_equal(_count_type(&model, IEMS_BMSER_TYPE_U16), 95);
    assert_int_equal(_count_type(&model, IEMS_BMSER_TYPE_S16), 9);
    assert_int_equal(_count_type(&model, IEMS_BMSER_TYPE_U32), 11);
    assert_int_equal(_count_type(&model, IEMS_BMSER_TYPE_S32), 55);
    assert_int_equal(_count_type(&model, IEMS_BMSER_TYPE_FLOAT), 0);
    assert_int_equal(_count_type(&model, IEMS_BMSER_TYPE_BI16), 0);
    assert_int_equal(_count_cycle(&model, 1000), 7);
    assert_int_equal(_count_cycle(&model, 0), 2);
    assert_int_equal(_count_scaled(&model, 1.0), 93);
    assert_int_equal(_count_scaled(&model, 10.0), 69);
    assert_int_equal(_count_scaled(&model, 100.0), 1);
    assert_int_equal(_count_scaled(&model, 1000.0), 7);
    assert_int_equal(_count_nodesc(&model), 75);

    const iems_bmser_dev_t *ems = &model.devs[0];
    assert_string_equal(ems->dev_id, "ems");
    assert_string_equal(ems->module, "env");
    assert_string_equal(ems->type, "emsctrl");
    assert_int_equal(ems->block_count, 3);
    assert_int_equal(ems->blocks[0].block_id, 0x8001);
    assert_int_equal(ems->blocks[0].cycle_ms, 1000);
    assert_int_equal(ems->blocks[0].sub_count, 8);

    const iems_bmser_sub_t *first = &ems->blocks[0].subs[0];
    assert_string_equal(first->name, "系统运行状态");
    assert_int_equal(first->type, IEMS_BMSER_TYPE_U16);
    assert_string_equal(first->tag, "SysStatus");
    assert_true(first->scale == 1.0);

    const iems_bmser_dev_t *pcs = &model.devs[1];
    assert_string_equal(pcs->dev_id, "pcs456");
    assert_string_equal(pcs->module, "ems");
    assert_string_equal(pcs->type, "pcsdata");
    assert_int_equal(pcs->block_count, 6);
    assert_int_equal(pcs->blocks[0].block_id, 0x8001);
    assert_int_equal(pcs->blocks[0].cycle_ms, 0);
    assert_int_equal(pcs->blocks[5].block_id, 0x8031);
    assert_int_equal(pcs->blocks[5].cycle_ms, 0);
    assert_int_equal(pcs->blocks[4].sub_count, 34);

    /* The shipped file repeats tags. The model preserves every slot rather
     * than deduplicating them. */
    assert_int_equal(_count_tag(&model, "ems", "GridPower"), 2);
    assert_int_equal(_count_tag(&model, "pcs456", "operation_mode"), 2);
    assert_int_equal(_count_tag(&model, "ems", "place1"), 12);

    iems_bmser_ipc_free(&model);
    assert_null(model.devs);
    assert_int_equal(model.sub_count, 0);
}

/* ------------------------------------------------------------------ */
/* Accepted documents                                                 */
/* ------------------------------------------------------------------ */

static void test_accepts_optional_and_bi16(void **state) {
    (void)state;
    const char *doc =
        "{\"schema_version\":\"1.0.0\",\"dev\":[{\"dev_id\":\"d\",\"module\":\"m\","
        "\"type\":\"t\",\"blocks\":[{\"block_id\":\"32769\",\"cycle\":0,"
        "\"sub_id\":[{\"name\":\"label\",\"type\":\"BI16\",\"tag\":\"bits\",\"scale\":1}]}]}]}";
    iems_bmser_ipc_t model = {0};
    char err[256];
    assert_int_equal(_load_mem(doc, &model, err, sizeof(err)), 0);
    assert_int_equal(model.dev_count, 1);
    assert_int_equal(model.devs[0].blocks[0].block_id, 32769);
    assert_int_equal(model.devs[0].blocks[0].cycle_ms, 0);
    assert_int_equal(model.devs[0].blocks[0].subs[0].type, IEMS_BMSER_TYPE_BI16);
    assert_string_equal(model.devs[0].blocks[0].subs[0].name, "label");
    assert_int_equal(model.placeholder_count, 0);
    assert_string_equal(iems_bmser_type_name(IEMS_BMSER_TYPE_BI16), "BI16");
    assert_string_equal(iems_bmser_type_name(IEMS_BMSER_TYPE_U16), "U16");
    assert_null(iems_bmser_type_name(IEMS_BMSER_TYPE_ERROR));
    iems_bmser_ipc_free(&model);
}

static void test_accepts_default_path_name(void **state) {
    (void)state;
    /* NULL relative_path selects the default; the in-memory source ignores it. */
    iems_bmser_ipc_t model = {0};
    char err[256];
    assert_int_equal(_load_mem("{\"dev\":[]}", &model, err, sizeof(err)), 0);
    assert_int_equal(model.dev_count, 0);
    assert_true(model.loaded);
    iems_bmser_ipc_free(&model);
}

/* ------------------------------------------------------------------ */
/* Rejected documents                                                 */
/* ------------------------------------------------------------------ */

static void test_rejects_shape_and_keys(void **state) {
    (void)state;
    _expect_reject_doc("{}", "MISSING_REQUIRED");
    _expect_reject_doc("[]", "INVALID_ROOT");
    _expect_reject_doc("{\"dev\":{}}", "INVALID_TYPE");
    _expect_reject_doc("{\"dev\":[],\"extra\":1}", "UNKNOWN_KEY");
    _expect_reject_doc("{\"schema_version\":\"2.0.0\",\"dev\":[]}", "UNSUPPORTED_SCHEMA_VERSION");
    _expect_reject_doc("{\"dev\":[],\"dev\":[]}", "DUPLICATE_KEY");
    _expect_reject_doc("{\"dev\":[]} trailing", "INVALID_JSON");
    _expect_reject_dev("{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\"}", "MISSING_REQUIRED");
    _expect_reject_dev("{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":[],\"x\":1}",
                       "UNKNOWN_KEY");
    _expect_reject_dev("{\"dev_id\":\"\",\"module\":\"m\",\"type\":\"t\",\"blocks\":[]}",
                       "INVALID_VALUE");
    _expect_reject_dev("{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":{}}",
                       "INVALID_TYPE");
}

static void test_rejects_block_fields(void **state) {
    (void)state;
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0xZZ\",\"cycle\":1000,\"sub_id\":[" SUB_BASE "]}]}",
        "INVALID_VALUE");
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x123456789\",\"cycle\":1000,\"sub_id\":[" SUB_BASE "]}]}",
        "INVALID_VALUE");
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x1\",\"cycle\":1000,\"sub_id\":[" SUB_BASE "],\"x\":1}]}",
        "UNKNOWN_KEY");
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x1\",\"sub_id\":[" SUB_BASE "]}]}",
        "MISSING_REQUIRED");
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x1\",\"cycle\":500,\"sub_id\":[" SUB_BASE "]}]}",
        "INVALID_VALUE");
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x1\",\"cycle\":1500,\"sub_id\":[" SUB_BASE "]}]}",
        "INVALID_VALUE");
    _expect_reject_dev("{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":[" BLOCK_BASE
                       "," BLOCK_BASE "]}",
                       "DUPLICATE_BLOCK_ID");
    _expect_reject_doc("{\"dev\":[" DEV_BASE "," DEV_BASE "]}", "DUPLICATE_DEV_ID");
}

static void test_rejects_sub_fields(void **state) {
    (void)state;
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x1\",\"cycle\":1000,\"sub_id\":["
        "{\"type\":\"DOUBLE\",\"tag\":\"x\",\"scale\":1}]}]}",
        "INVALID_VALUE");
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x1\",\"cycle\":1000,\"sub_id\":[{\"type\":\"U16\",\"scale\":1}]}]}",
        "MISSING_REQUIRED");
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x1\",\"cycle\":1000,\"sub_id\":["
        "{\"type\":\"U16\",\"tag\":\"\",\"scale\":1}]}]}",
        "INVALID_VALUE");
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x1\",\"cycle\":1000,\"sub_id\":["
        "{\"type\":\"U16\",\"tag\":\"x\"}]}]}",
        "MISSING_REQUIRED");
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x1\",\"cycle\":1000,\"sub_id\":["
        "{\"type\":\"U16\",\"tag\":\"x\",\"scale\":0}]}]}",
        "INVALID_VALUE");
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x1\",\"cycle\":1000,\"sub_id\":["
        "{\"type\":\"U16\",\"tag\":\"x\",\"scale\":\"1\"}]}]}",
        "INVALID_TYPE");
    _expect_reject_dev(
        "{\"dev_id\":\"d\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
        "{\"block_id\":\"0x1\",\"cycle\":1000,\"sub_id\":["
        "{\"type\":\"U16\",\"tag\":\"x\",\"scale\":1,\"extra\":0}]}]}",
        "UNKNOWN_KEY");

    /* The v3 buffer for "<dev_id>.<tag>" is 64 bytes. */
    char id[64];
    memset(id, 'a', sizeof(id) - 1);
    id[sizeof(id) - 1] = '\0';
    char dev[512];
    int n = snprintf(dev, sizeof(dev),
                     "{\"dev_id\":\"%s\",\"module\":\"m\",\"type\":\"t\",\"blocks\":["
                     "{\"block_id\":\"0x1\",\"cycle\":1000,\"sub_id\":["
                     "{\"type\":\"U16\",\"tag\":\"abcdef\",\"scale\":1}]}]}",
                     id);
    assert_true(n > 0 && (size_t)n < sizeof(dev));
    _expect_reject_dev(dev, "VALUE_TOO_LONG");
}

static void test_rejects_missing_file_and_bad_args(void **state) {
    (void)state;
    iems_bmser_ipc_t model = {0};
    char err[256];
    assert_int_equal(_load_mem(NULL, &model, err, sizeof(err)), -1);
    assert_true(strstr(err, "MISSING_REQUIRED") != NULL);
    assert_null(model.devs);

    mem_source_t ctx = {"{\"dev\":[]}"};
    iems_config_source_t source = {_mem_read, NULL, &ctx};
    assert_int_equal(iems_bmser_ipc_load(NULL, NULL, &model, err, sizeof(err)), -1);
    assert_true(strstr(err, "INVALID_ARGUMENT") != NULL);
    assert_int_equal(iems_bmser_ipc_load(&source, NULL, NULL, err, sizeof(err)), -1);
    assert_true(strstr(err, "INVALID_ARGUMENT") != NULL);

    iems_bmser_ipc_free(NULL);
    iems_bmser_ipc_free(&model);
    iems_bmser_ipc_free(&model);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_real_file_counts),
        cmocka_unit_test(test_accepts_optional_and_bi16),
        cmocka_unit_test(test_accepts_default_path_name),
        cmocka_unit_test(test_rejects_shape_and_keys),
        cmocka_unit_test(test_rejects_block_fields),
        cmocka_unit_test(test_rejects_sub_fields),
        cmocka_unit_test(test_rejects_missing_file_and_bad_args),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
