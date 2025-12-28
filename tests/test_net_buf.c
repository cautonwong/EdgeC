#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "net_buf.h"

// Mock destroy callback
static int destroy_count = 0;
static void on_destroy(net_buf_t *buf) {
    (void)buf;
    destroy_count++;
}

typedef struct {
    net_buf_pool_t pool;
} test_pool_ctx_t;

static int pool_setup(void **state) {
    test_pool_ctx_t *ctx = calloc(1, sizeof(test_pool_ctx_t));
    net_buf_pool_init(&ctx->pool, 10, 128, on_destroy, NULL);
    destroy_count = 0;
    *state = ctx;
    return 0;
}

static int pool_teardown(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_pool_destroy(&ctx->pool);
    free(ctx);
    return 0;
}

// 1. 测试池初始化与销毁
static void test_pool_init_destroy(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 10, 128, on_destroy, NULL), 0);
    assert_non_null(pool.storage);
    net_buf_pool_destroy(&pool);
}

// 2. 测试分配与引用计数
static void test_alloc_ref(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_pool_t *pool = &ctx->pool;

    net_buf_t *buf1 = net_buf_alloc(pool, 0);
    assert_non_null(buf1);
    net_buf_t *buf2 = net_buf_alloc(pool, 0);
    assert_non_null(buf2);

    net_buf_ref(buf1);
    assert_int_equal(buf1->ref, 2);

    net_buf_unref(buf1);
    assert_int_equal(destroy_count, 0);
    net_buf_unref(buf1);
    assert_int_equal(destroy_count, 1);
    net_buf_unref(buf2);
    assert_int_equal(destroy_count, 2);
}

// 3. 测试数据操作
static void test_data_manipulation(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_pool_t *pool = &ctx->pool;

    net_buf_t *buf = net_buf_alloc_with_data(pool, 20, 0);
    assert_int_equal(net_buf_headroom(buf), 20);

    net_buf_add(buf, 10);
    assert_int_equal(buf->len, 10);

    net_buf_push(buf, 5);
    assert_int_equal(buf->len, 15);

    net_buf_pull(buf, 5);
    assert_int_equal(buf->len, 10);

    net_buf_unref(buf);
}

// 4. 测试分片 (Fragmentation) - 修复版
static void test_fragmentation(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_pool_t *pool = &ctx->pool;

    net_buf_t *head = net_buf_alloc(pool, 0);
    net_buf_t *frag1 = net_buf_alloc(pool, 0);
    net_buf_t *frag2 = net_buf_alloc(pool, 0);

    // head -> frag1 -> frag2
    net_buf_frag_add(head, frag1);
    net_buf_frag_add(head, frag2);

    assert_ptr_equal(head->frags, frag1);
    assert_ptr_equal(frag1->next, frag2);

    // Insert frag_mid after frag1: head -> frag1 -> frag_mid -> frag2
    net_buf_t *frag_mid = net_buf_alloc(pool, 0);
    net_buf_frag_insert(frag1, frag_mid);

    assert_ptr_equal(frag1->next, frag_mid);
    assert_ptr_equal(frag_mid->next, frag2);

    // Delete frag1 from head: head -> frag_mid -> frag2
    net_buf_t *removed = net_buf_frag_del(head, frag1);
    assert_ptr_equal(removed, frag1);
    assert_ptr_equal(head->frags, frag_mid);
    assert_ptr_equal(frag_mid->next, frag2);

    // Cleanup removed frag
    net_buf_unref(removed);  // destroy_count -> 1

    // Cleanup head (recursively unrefs frag_mid and frag2)
    net_buf_unref(head);  // destroy_count -> 1+3 = 4

    assert_int_equal(destroy_count, 4);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pool_init_destroy),
        cmocka_unit_test_setup_teardown(test_alloc_ref, pool_setup, pool_teardown),
        cmocka_unit_test_setup_teardown(test_data_manipulation, pool_setup, pool_teardown),
        cmocka_unit_test_setup_teardown(test_fragmentation, pool_setup, pool_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
