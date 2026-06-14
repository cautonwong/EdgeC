#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_os_port.h"

static void dummy_thread_entry(void* arg) {
    int* counter = (int*)arg;
    (*counter)++;
}

static void test_os_port_abstraction(void** state) {
    (void)state;

    // 1. Test Memory Abstraction
    void* ptr = EDGE_MALLOC(1024);
    assert_non_null(ptr);
    memset(ptr, 0xAA, 1024);
    EDGE_FREE(ptr);

    void* ptr_c = EDGE_CALLOC(10, 128);
    assert_non_null(ptr_c);
    // Should be zeroed
    for (int i = 0; i < 1280; i++) {
        assert_int_equal(((uint8_t*)ptr_c)[i], 0);
    }
    EDGE_FREE(ptr_c);

    // 2. Test Mutex Abstraction
    edge_mutex_t mutex;
    assert_int_equal(edge_mutex_init(&mutex), 0);
    assert_int_equal(edge_mutex_lock(&mutex), 0);
    assert_int_equal(edge_mutex_unlock(&mutex), 0);
    assert_int_equal(edge_mutex_destroy(&mutex), 0);

    // 3. Test Thread Abstraction
    edge_thread_t thread;
    int counter = 0;
    assert_int_equal(edge_thread_create(&thread, dummy_thread_entry, &counter), 0);
    assert_int_equal(edge_thread_join(&thread), 0);
    assert_int_equal(counter, 1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_os_port_abstraction),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
