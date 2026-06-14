#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uv.h>

#include "shmring.h"

#define TEST_RING_NAME "test_memfd_bus"
#define TEST_SLOT_COUNT 1024
#define TEST_WRITE_COUNT 500

static int setup(void** state) {
    shm_ring_t* ring = malloc(sizeof(shm_ring_t));
    // memfd_create 不需要 unlink
    if (shmring_create(ring, TEST_RING_NAME, TEST_SLOT_COUNT) != 0) {
        free(ring);
        return -1;
    }
    *state = ring;
    return 0;
}

static int teardown(void** state) {
    shm_ring_t* ring = (shm_ring_t*)*state;
    shmring_destroy(ring);
    free(ring);
    return 0;
}

static void test_single_thread_read_write(void** state) {
    shm_ring_t* ring = (shm_ring_t*)*state;
    PipeMsg write_msg = {0}, read_msg = {0};
    uint64_t seq = 0;

    write_msg.tag_id = 101;
    write_msg.payload.val.f = 99.9;
    shm_ring_write(ring, &write_msg);

    assert_int_equal(shm_ring_read(ring, &seq, &read_msg), 0);
    assert_float_equal(read_msg.payload.val.f, 99.9, 0.001);
}

static void test_multi_process_concurrency(void** state) {
    shm_ring_t* ring_parent = (shm_ring_t*)*state;

    pid_t pid = fork();

    if (pid == 0) {
        // 子进程：Writer

        // 直接利用继承的 ring_parent 指针即可，因为 mmap 是 MAP_SHARED

        for (int i = 0; i < TEST_WRITE_COUNT; i++) {
            PipeMsg msg = {0};

            msg.payload.val.i = i;

            shm_ring_write(ring_parent, &msg);

            // 极高频模拟

            if (i % 50 == 0) usleep(10);
        }

        exit(0);

    } else {
        // 父进程：Reader

        PipeMsg msg;

        uint64_t seq = 0;

        int count = 0;

        int timeout_ms = 2000;  // 2秒超时

        while (count < TEST_WRITE_COUNT && timeout_ms > 0) {
            if (shm_ring_read(ring_parent, &seq, &msg) == 0) {
                count++;

            } else {
                usleep(1000);

                timeout_ms -= 1;
            }
        }

        int status;
        waitpid(pid, &status, 0);
        assert_int_equal(count, TEST_WRITE_COUNT);
        printf("[Test] Read all %d items via anonymous memfd successfully\n", count);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_single_thread_read_write, setup, teardown),
        cmocka_unit_test_setup_teardown(test_multi_process_concurrency, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}