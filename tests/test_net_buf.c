#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "net_buf.h"

// 1. 测试基本的内存池分配与释放
static void test_net_buf_pool_basic(void** state) {
    (void)state;
    struct net_buf_pool* pool = net_buf_pool_new(5, 128, 0);
    assert_non_null(pool);

    net_buf_t* buf = net_buf_alloc(pool, 0);
    assert_non_null(buf);
    assert_int_equal(buf->b.size, 128);
    assert_int_equal(buf->b.len, 0);

    net_buf_unref(buf);
    net_buf_pool_unref(pool);
}

// 2. 测试引用计数逻辑
static void test_net_buf_ref_count(void** state) {
    (void)state;
    struct net_buf_pool* pool = net_buf_pool_new(2, 64, 0);
    net_buf_t* buf = net_buf_alloc(pool, 0);

    assert_int_equal(buf->ref, 1);

    net_buf_t* buf2 = net_buf_ref(buf);
    assert_ptr_equal(buf, buf2);
    assert_int_equal(buf->ref, 2);

    net_buf_unref(buf);
    assert_int_equal(buf->ref, 1);

    net_buf_unref(buf2);  // 真正释放回池
    net_buf_pool_unref(pool);
}

// 3. 测试 simple 缓冲区的推入和拉取
static void test_net_buf_simple_ops(void** state) {
    (void)state;
    struct net_buf_pool* pool = net_buf_pool_new(2, 64, 0);
    net_buf_t* buf = net_buf_alloc(pool, 0);

    // Push 4 bytes
    uint32_t val = 0xDEADBEEF;
    net_buf_simple_add_le32(&buf->b, val);
    assert_int_equal(buf->b.len, 4);

    // Pull 4 bytes
    uint32_t read_val = net_buf_simple_pull_le32(&buf->b);
    assert_int_equal(read_val, val);
    assert_int_equal(buf->b.len, 0);

    net_buf_unref(buf);
    net_buf_pool_unref(pool);
}

// 4. 测试空间不足时的安全检查
static void test_net_buf_overflow_protection(void** state) {
    (void)state;
    struct net_buf_pool* pool = net_buf_pool_new(1, 16, 0);
    net_buf_t* buf = net_buf_alloc(pool, 0);

    // Push 16 bytes (OK)
    void* ptr = net_buf_simple_add(&buf->b, 16);
    assert_non_null(ptr);
    assert_int_equal(buf->b.len, 16);

    net_buf_unref(buf);
    net_buf_pool_unref(pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_net_buf_pool_basic),
        cmocka_unit_test(test_net_buf_ref_count),
        cmocka_unit_test(test_net_buf_simple_ops),
        cmocka_unit_test(test_net_buf_overflow_protection),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
