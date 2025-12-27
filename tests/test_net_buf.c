#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include "core/net_buf.h"
#include <string.h>

// Mock destroy callback
static int destroy_count = 0;
static void on_destroy(net_buf_t *buf) {
    (void)buf;
    destroy_count++;
}

// 1. 测试池初始化与销毁
static void test_pool_init_destroy(void **state) {
    (void)state;
    net_buf_pool_t pool;
    // 正常初始化
    assert_int_equal(net_buf_pool_init(&pool, 10, 128, on_destroy, NULL), 0);
    assert_non_null(pool.storage);
    assert_int_equal(pool.uninit_count, 10);
    
    // 销毁
    net_buf_pool_destroy(&pool);
    assert_null(pool.storage);

    // 异常参数测试
    assert_int_not_equal(net_buf_pool_init(NULL, 10, 128, NULL, NULL), 0);
    assert_int_not_equal(net_buf_pool_init(&pool, 0, 128, NULL, NULL), 0);
}

// 2. 测试分配与引用计数
static void test_alloc_ref(void **state) {
    (void)state;
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 2, 128, on_destroy, NULL);
    destroy_count = 0;

    // 分配第一个
    net_buf_t *buf1 = net_buf_alloc(&pool, 0);
    assert_non_null(buf1);
    assert_int_equal(buf1->ref, 1);
    assert_int_equal(pool.uninit_count, 1);

    // 分配第二个
    net_buf_t *buf2 = net_buf_alloc(&pool, 0);
    assert_non_null(buf2);
    
    // 池已满，非阻塞分配应返回 NULL
    net_buf_t *buf3 = net_buf_alloc(&pool, 0);
    assert_null(buf3);

    // 测试引用计数增加
    net_buf_ref(buf1);
    assert_int_equal(buf1->ref, 2);

    // 测试引用计数减少
    net_buf_unref(buf1); // ref -> 1
    assert_int_equal(destroy_count, 0); // 还没销毁

    net_buf_unref(buf1); // ref -> 0, 回收
    assert_int_equal(destroy_count, 1);

    // 回收后应该能再次分配
    buf3 = net_buf_alloc(&pool, 0);
    assert_non_null(buf3);

    net_buf_pool_destroy(&pool);
}

// 3. 测试 Headroom/Tailroom 和数据操作
static void test_data_manipulation(void **state) {
    (void)state;
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 1, 100, NULL, NULL);

    // 分配并预留 20 字节头部
    net_buf_t *buf = net_buf_alloc_with_data(&pool, 20, 0);
    assert_non_null(buf);
    assert_int_equal(net_buf_headroom(buf), 20);
    assert_int_equal(net_buf_tailroom(buf), 80);
    assert_int_equal(buf->len, 0);

    // 添加数据 (Append)
    char *data1 = (char *)net_buf_add(buf, 10);
    strcpy(data1, "123456789");
    assert_int_equal(buf->len, 10);
    assert_int_equal(net_buf_tailroom(buf), 70);

    // 头部推入 (Prepend) - 模拟加协议头
    char *head = (char *)net_buf_push(buf, 5);
    memcpy(head, "HEAD:", 5);
    assert_int_equal(buf->len, 15);
    assert_int_equal(net_buf_headroom(buf), 15);

    // 头部移除 (Consume) - 模拟解析协议头
    char *pulled = (char *)net_buf_pull(buf, 5);
    assert_ptr_equal(pulled, head);
    assert_int_equal(buf->len, 10);
    
    // Memcpy 添加
    assert_int_equal(net_buf_add_mem(buf, "ABC", 3), 0);
    assert_int_equal(buf->len, 13);

    // 边界测试：添加超过 tailroom
    assert_int_not_equal(net_buf_add_mem(buf, "TOO_LONG", 100), 0);

    net_buf_unref(buf);
    net_buf_pool_destroy(&pool);
}

// 4. 测试分片 (Fragmentation)
static void test_fragmentation(void **state) {
    (void)state;
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 5, 10, NULL, NULL); // 小 buffer 强制分片

    net_buf_t *head = net_buf_alloc(&pool, 0);
    net_buf_t *frag1 = net_buf_alloc(&pool, 0);
    net_buf_t *frag2 = net_buf_alloc(&pool, 0);

    // 链表拼接
    net_buf_frag_add(head, frag1);
    net_buf_frag_add(head, frag2);

    assert_ptr_equal(head->frags, frag1);
    assert_ptr_equal(frag1->next, frag2);
    assert_ptr_equal(net_buf_frag_last(head), frag2);

    // 插入
    net_buf_t *frag_mid = net_buf_alloc(&pool, 0);
    net_buf_frag_insert(head, frag_mid); // 插入到 head 之后
    assert_ptr_equal(head->frags, frag_mid);
    assert_ptr_equal(frag_mid->next, frag1);

    // 删除
    net_buf_t *removed = net_buf_frag_del(head, frag_mid);
    assert_ptr_equal(removed, frag_mid);
    assert_ptr_equal(head->frags, frag1);
    net_buf_unref(removed);

    // 销毁链表 (head unref 应该递归 unref frags)
    destroy_count = 0;
    pool.destroy_cb = on_destroy;
    net_buf_unref(head);
    
    // head + frag1 + frag2 = 3 个 buffer 应该被销毁
    assert_int_equal(destroy_count, 3);

    net_buf_pool_destroy(&pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pool_init_destroy),
        cmocka_unit_test(test_alloc_ref),
        cmocka_unit_test(test_data_manipulation),
        cmocka_unit_test(test_fragmentation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
