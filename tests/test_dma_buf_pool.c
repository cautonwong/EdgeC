/**
 * @file test_dma_buf_pool.c
 * @brief Unit & Integration Tests for Linux DMA-BUF Memory Pool & Lifecycle Manager (AEP-746 /
 * Issue #276)
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

#include "dma_buf_pool.h"
#include "npu_hal.h"

/* -------------------------------------------------------------------------- */
/* Test Cases                                                                 */
/* -------------------------------------------------------------------------- */

static void test_dma_buf_pool_lifecycle(void **state) {
    (void)state;

    dma_buf_pool_t *pool = NULL;
    dma_buf_pool_config_t cfg = {
        .buffer_size = 4096, .pool_capacity = 8, .enable_auto_flush = true, .alignment = 64};

    assert_int_equal(dma_buf_pool_init(&pool, &cfg), 0);
    assert_non_null(pool);

    dma_buf_pool_stats_t stats;
    assert_int_equal(dma_buf_pool_get_stats(pool, &stats), 0);
    assert_int_equal(stats.total_buffers, 8);
    assert_int_equal(stats.active_buffers, 0);
    assert_int_equal(stats.peak_buffers, 0);
    assert_int_equal(stats.total_memory_bytes, 8 * 4096);

    assert_int_equal(dma_buf_pool_destroy(pool), 0);
}

static void test_dma_buf_pool_acquire_and_release(void **state) {
    (void)state;

    dma_buf_pool_t *pool = NULL;
    dma_buf_pool_config_t cfg = {
        .buffer_size = 2048, .pool_capacity = 4, .enable_auto_flush = true, .alignment = 64};

    assert_int_equal(dma_buf_pool_init(&pool, &cfg), 0);

    dma_buf_desc_t *bufs[4];
    for (int i = 0; i < 4; i++) {
        assert_int_equal(dma_buf_pool_acquire(pool, &bufs[i]), 0);
        assert_non_null(bufs[i]);
        assert_non_null(bufs[i]->vaddr);
        assert_int_equal(bufs[i]->size, 2048);
        assert_true(bufs[i]->fd >= 100);
        assert_int_equal(bufs[i]->ref_count, 1);
    }

    /* 5th acquire should fail due to pool exhaustion */
    dma_buf_desc_t *extra = NULL;
    assert_int_equal(dma_buf_pool_acquire(pool, &extra), -3);
    assert_null(extra);

    dma_buf_pool_stats_t stats;
    assert_int_equal(dma_buf_pool_get_stats(pool, &stats), 0);
    assert_int_equal(stats.active_buffers, 4);
    assert_int_equal(stats.peak_buffers, 4);
    assert_int_equal(stats.total_allocations, 4);

    /* Release all */
    for (int i = 0; i < 4; i++) {
        assert_int_equal(dma_buf_unref(pool, bufs[i]), 0);
    }

    assert_int_equal(dma_buf_pool_get_stats(pool, &stats), 0);
    assert_int_equal(stats.active_buffers, 0);
    assert_int_equal(stats.total_releases, 4);

    assert_int_equal(dma_buf_pool_destroy(pool), 0);
}

static void test_dma_buf_ref_counting(void **state) {
    (void)state;

    dma_buf_pool_t *pool = NULL;
    dma_buf_pool_config_t cfg = {
        .buffer_size = 1024, .pool_capacity = 2, .enable_auto_flush = false, .alignment = 64};

    assert_int_equal(dma_buf_pool_init(&pool, &cfg), 0);

    dma_buf_desc_t *buf = NULL;
    assert_int_equal(dma_buf_pool_acquire(pool, &buf), 0);
    assert_int_equal(buf->ref_count, 1);

    /* Driver A shares buffer with Driver B */
    assert_int_equal(dma_buf_ref(buf), 0);
    assert_int_equal(buf->ref_count, 2);

    /* Driver A finishes and unrefs */
    assert_int_equal(dma_buf_unref(pool, buf), 0);
    assert_int_equal(buf->ref_count, 1);
    assert_true(buf->is_in_use);

    /* Driver B finishes and unrefs */
    assert_int_equal(dma_buf_unref(pool, buf), 0);
    assert_int_equal(buf->ref_count, 0);
    assert_false(buf->is_in_use);

    assert_int_equal(dma_buf_pool_destroy(pool), 0);
}

static void test_dma_buf_sync_operations(void **state) {
    (void)state;

    dma_buf_pool_t *pool = NULL;
    dma_buf_pool_config_t cfg = {
        .buffer_size = 512, .pool_capacity = 2, .enable_auto_flush = true, .alignment = 64};

    assert_int_equal(dma_buf_pool_init(&pool, &cfg), 0);

    dma_buf_desc_t *buf = NULL;
    assert_int_equal(dma_buf_pool_acquire(pool, &buf), 0);

    /* Test CPU write access */
    assert_int_equal(dma_buf_sync_start(buf, DMA_BUF_SYNC_WRITE), 0);
    memset(buf->vaddr, 0xAA, buf->size);
    assert_int_equal(dma_buf_sync_end(buf, DMA_BUF_SYNC_WRITE), 0);

    /* Test Device read access */
    assert_int_equal(dma_buf_sync_start(buf, DMA_BUF_SYNC_READ), 0);
    uint8_t *byte_ptr = (uint8_t *)buf->vaddr;
    assert_int_equal(byte_ptr[0], 0xAA);
    assert_int_equal(dma_buf_sync_end(buf, DMA_BUF_SYNC_READ), 0);

    assert_int_equal(dma_buf_unref(pool, buf), 0);
    assert_int_equal(dma_buf_pool_destroy(pool), 0);
}

static void test_dma_buf_to_npu_tensor_bridge(void **state) {
    (void)state;

    dma_buf_pool_t *pool = NULL;
    dma_buf_pool_config_t cfg = {.buffer_size = 256 * sizeof(float),
                                 .pool_capacity = 4,
                                 .enable_auto_flush = true,
                                 .alignment = 64};

    assert_int_equal(dma_buf_pool_init(&pool, &cfg), 0);

    dma_buf_desc_t *buf = NULL;
    assert_int_equal(dma_buf_pool_acquire(pool, &buf), 0);

    float *f_ptr = (float *)buf->vaddr;
    for (int i = 0; i < 256; i++) {
        f_ptr[i] = (float)i * 1.5f;
    }

    npu_tensor_buf_t tensor;
    assert_int_equal(dma_buf_to_npu_tensor(buf, &tensor), 0);

    assert_true(tensor.is_zero_copy);
    assert_ptr_equal(tensor.host_ptr, buf->vaddr);
    assert_ptr_equal(tensor.device_ptr, buf->vaddr);
    assert_int_equal(tensor.size, 256 * sizeof(float));
    assert_int_equal(tensor.dma_fd, buf->fd);
    assert_true((tensor.flags & NPU_MEM_FLAG_ZERO_COPY_SHM) != 0);

    assert_int_equal(dma_buf_unref(pool, buf), 0);
    assert_int_equal(dma_buf_pool_destroy(pool), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dma_buf_pool_lifecycle),
        cmocka_unit_test(test_dma_buf_pool_acquire_and_release),
        cmocka_unit_test(test_dma_buf_ref_counting),
        cmocka_unit_test(test_dma_buf_sync_operations),
        cmocka_unit_test(test_dma_buf_to_npu_tensor_bridge),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
