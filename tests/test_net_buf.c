#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "net_buf.h"

static int destroy_count = 0;
static void on_destroy(net_buf_t *buf) {
    (void)buf;
    destroy_count++;
}

typedef struct {
    net_buf_pool_t pool;
} test_pool_ctx_t;
typedef struct {
    uint32_t magic;
} test_meta_t;

static int pool_setup(void **state) {
    test_pool_ctx_t *ctx = calloc(1, sizeof(test_pool_ctx_t));
    net_buf_pool_init(&ctx->pool, 10, 128, sizeof(test_meta_t), on_destroy, NULL);
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

static void test_pool_init_destroy(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 10, 128, 0, on_destroy, NULL), 0);
    net_buf_pool_destroy(&pool);
}

static void test_alloc_ref(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_t *buf = net_buf_alloc(&ctx->pool, 0);
    assert_non_null(buf);
    net_buf_ref(buf);
    assert_int_equal(buf->ref, 2);
    net_buf_unref(buf);
    net_buf_unref(buf);
    assert_int_equal(destroy_count, 1);
}

static void test_data_push_pull(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_t *buf = net_buf_alloc(&ctx->pool, 0);
    net_buf_reserve(buf, 20);
    net_buf_add(buf, 10);
    assert_int_equal(buf->b.len, 10);
    void *head = net_buf_push(buf, 5);
    assert_ptr_equal(head, buf->b.data);
    assert_int_equal(buf->b.len, 15);
    net_buf_unref(buf);
}

static void test_fragment_chain(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_t *h = net_buf_alloc(&ctx->pool, 0);
    net_buf_t *f1 = net_buf_alloc(&ctx->pool, 0);
    net_buf_t *f2 = net_buf_alloc(&ctx->pool, 0);
    net_buf_frag_add(h, f1);
    net_buf_frag_add(h, f2);
    assert_ptr_equal(h->frags, f1);
    assert_ptr_equal(f1->frags, f2);
    assert_ptr_equal(net_buf_frag_last(h), f2);
    net_buf_unref(h);
    assert_int_equal(destroy_count, 3);
}

static void test_net_buf_reuse(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_t *bufs[10];
    for (int i = 0; i < 10; i++) bufs[i] = net_buf_alloc(&ctx->pool, 0);
    assert_null(net_buf_alloc(&ctx->pool, 0));
    net_buf_t *b0_ptr = bufs[0];
    net_buf_unref(bufs[0]);
    net_buf_t *retry = net_buf_alloc(&ctx->pool, 0);
    assert_ptr_equal(b0_ptr, retry);
    net_buf_unref(retry);
    for (int i = 1; i < 10; i++) net_buf_unref(bufs[i]);
}

static void test_net_buf_linearize_advanced(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_t *h = net_buf_alloc(&ctx->pool, 0);
    net_buf_t *f = net_buf_alloc(&ctx->pool, 0);
    net_buf_simple_add_mem(&h->b, "ABC", 3);
    net_buf_simple_add_mem(&f->b, "DEF", 3);
    net_buf_frag_add(h, f);
    uint8_t dst[10] = {0};
    size_t n = net_buf_linearize(dst, sizeof(dst), h, 4, 2);
    assert_int_equal(n, 2);
    assert_memory_equal(dst, "EF", 2);
    net_buf_unref(h);
}

static void test_clone_and_endian(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_t *buf = net_buf_alloc(&ctx->pool, 0);
    net_buf_simple_add_be64(&buf->b, 0x0123456789ABCDEF);
    net_buf_t *clone = net_buf_clone(buf, 0);
    assert_int_equal(net_buf_simple_pull_be64(&clone->b), 0x0123456789ABCDEF);
    net_buf_unref(clone);
    net_buf_unref(buf);
}

static void test_user_data_and_id(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_t *buf = net_buf_alloc(&ctx->pool, 0);
    test_meta_t *meta = net_buf_user_data(buf);
    meta->magic = 0xDEADBEEF;
    uint16_t id = net_buf_id(buf);
    assert_ptr_equal(net_buf_pool_get(&ctx->pool, id), buf);
    net_buf_unref(buf);
}

static void test_reset_and_pull_mem(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_t *buf = net_buf_alloc(&ctx->pool, 0);
    net_buf_simple_add_mem(&buf->b, "123456", 6);
    char dst[4] = {0};
    net_buf_simple_pull_mem(&buf->b, dst, 3);
    net_buf_simple_reset(&buf->b);
    assert_int_equal(buf->b.len, 0);
    net_buf_unref(buf);
}

static void test_frag_del_advanced(void **state) {
    test_pool_ctx_t *ctx = *state;

    net_buf_t *h = net_buf_alloc(&ctx->pool, 0);

    net_buf_t *f1 = net_buf_alloc(&ctx->pool, 0);

    net_buf_t *f2 = net_buf_alloc(&ctx->pool, 0);

    net_buf_t *f3 = net_buf_alloc(&ctx->pool, 0);

    // 构造链: h -> f1 -> f2 -> f3

    net_buf_frag_add(h, f1);

    net_buf_frag_add(h, f2);

    net_buf_frag_add(h, f3);

    // 1. 击穿深层搜索分支: 删除 f3 (前驱是 f2)

    net_buf_t *rem3 = net_buf_frag_del(h, f3);

    assert_ptr_equal(rem3, f3);

    // 2. 击穿直接分片分支: 删除 f1

    net_buf_t *rem1 = net_buf_frag_del(h, f1);

    assert_ptr_equal(rem1, f1);

    // 3. 击穿未找到分支 (命中末尾 return NULL): 构造一个孤立节点尝试从 h 中删除

    net_buf_t *isolated = net_buf_alloc(&ctx->pool, 0);

    assert_null(net_buf_frag_del(h, isolated));

    net_buf_unref(isolated);

    net_buf_unref(rem1);
    net_buf_unref(rem3);

    net_buf_unref(f2);  // 释放链条中残存的 f2

    net_buf_unref(h);
}

static void _releaser_thread(void *arg) {
    uv_sleep(100);
    net_buf_unref((net_buf_t *)arg);
}
static void test_alloc_blocking(void **state) {
    test_pool_ctx_t *ctx = *state;
    net_buf_t *bufs[10];
    for (int i = 0; i < 10; i++) bufs[i] = net_buf_alloc(&ctx->pool, 0);
    uv_thread_t tid;
    uv_thread_create(&tid, _releaser_thread, bufs[0]);
    net_buf_t *retry = net_buf_alloc(&ctx->pool, -1);
    assert_ptr_equal(retry, bufs[0]);
    uv_thread_join(&tid);
    net_buf_unref(retry);
    for (int i = 1; i < 10; i++) net_buf_unref(bufs[i]);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pool_init_destroy),
        cmocka_unit_test_setup_teardown(test_alloc_ref, pool_setup, pool_teardown),
        cmocka_unit_test_setup_teardown(test_data_push_pull, pool_setup, pool_teardown),
        cmocka_unit_test_setup_teardown(test_fragment_chain, pool_setup, pool_teardown),
        cmocka_unit_test_setup_teardown(test_net_buf_reuse, pool_setup, pool_teardown),
        cmocka_unit_test_setup_teardown(test_net_buf_linearize_advanced, pool_setup, pool_teardown),
        cmocka_unit_test_setup_teardown(test_clone_and_endian, pool_setup, pool_teardown),
        cmocka_unit_test_setup_teardown(test_user_data_and_id, pool_setup, pool_teardown),
        cmocka_unit_test_setup_teardown(test_reset_and_pull_mem, pool_setup, pool_teardown),
        cmocka_unit_test_setup_teardown(test_frag_del_advanced, pool_setup, pool_teardown),
        cmocka_unit_test_setup_teardown(test_alloc_blocking, pool_setup, pool_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}