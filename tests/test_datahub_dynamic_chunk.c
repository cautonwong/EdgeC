#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datahub.h"

static void test_datahub_chunk_allocation_and_schema_switch(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;

    int ret = datahub_init(&hub, loop, "test_chunk_bus", 32, 100, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(ret, 0);

    // 验证动态 Chunk 分配
    datahub_chunk_t chunk1;
    ret = datahub_allocate_chunk(&hub, 200, &chunk1);
    assert_int_equal(ret, 0);
    assert_non_null(chunk1.ptr);
    assert_int_equal(chunk1.capacity, 200);
    assert_int_equal(hub.chunk_count, 1);

    // 验证 Schema 切换 (Schema v1 -> Schema v2)
    schema_version_t schema_v1 = {
        .version = 1, .chunk_count = 1, .chunks = {chunk1}, .tag_count = 100};

    ret = datahub_switch_schema(&hub, &schema_v1);
    assert_int_equal(ret, 0);

    schema_version_t *active = (schema_version_t *)atomic_load(&hub.active_schema);
    assert_non_null(active);
    assert_int_equal(active->version, 1);
    assert_int_equal(active->tag_count, 100);

    // 再动态扩展 Chunk 2
    datahub_chunk_t chunk2;
    ret = datahub_allocate_chunk(&hub, 300, &chunk2);
    assert_int_equal(ret, 0);
    assert_int_equal(hub.chunk_count, 2);

    schema_version_t schema_v2 = {
        .version = 2, .chunk_count = 2, .chunks = {chunk1, chunk2}, .tag_count = 500};

    ret = datahub_switch_schema(&hub, &schema_v2);
    assert_int_equal(ret, 0);

    active = (schema_version_t *)atomic_load(&hub.active_schema);
    assert_non_null(active);
    assert_int_equal(active->version, 2);
    assert_int_equal(active->tag_count, 500);

    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_datahub_chunk_allocation_and_schema_switch),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
