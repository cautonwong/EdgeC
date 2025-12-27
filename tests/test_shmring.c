#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <cmocka.h>
#include <uv.h>
#include "shmring.h"

#define TEST_RING_NAME "/test_shmring_v2"
#define TEST_SLOT_COUNT 256
#define TEST_WRITE_COUNT 500

static int setup(void **state) {
    shm_ring_t *ring = malloc(sizeof(shm_ring_t));
    shm_unlink(TEST_RING_NAME);
    if (shmring_create(ring, TEST_RING_NAME, TEST_SLOT_COUNT) != 0) {
        free(ring);
        return -1;
    }
    *state = ring;
    return 0;
}

static int teardown(void **state) {
    shm_ring_t *ring = (shm_ring_t *)*state;
    shmring_destroy(ring);
    free(ring);
    return 0;
}

static void test_single_thread_read_write(void **state) {
    shm_ring_t *ring = (shm_ring_t *)*state;
    PipeMsg write_msg = {0}, read_msg = {0};
    uint64_t seq = 0;

    write_msg.tag_id = 101;
    write_msg.payload.val_f = 99.9;
    shm_ring_write(ring, &write_msg);

    assert_int_equal(shm_ring_read(ring, &seq, &read_msg), 0);
    assert_float_equal(read_msg.payload.val_f, 99.9, 0.001);
}

static void test_multi_process_concurrency(void **state) {
    (void)state;
    pid_t pid = fork();
    
    if (pid == 0) {
        // 子进程：Writer
        shm_ring_t ring;
        shmring_open(&ring, TEST_RING_NAME);
        for (int i = 0; i < TEST_WRITE_COUNT; i++) {
            PipeMsg msg = {0};
            msg.payload.val_i = i;
            shm_ring_write(&ring, &msg);
            usleep(1000); 
        }
        shmring_close(&ring);
        exit(0);
    } else {
        // 父进程：Reader
        shm_ring_t *ring = (shm_ring_t *)*state;
        PipeMsg msg;
        uint64_t seq = 0;
        int count = 0;
        
        for (int i = 0; i < 1000; i++) {
            if (shm_ring_read(ring, &seq, &msg) == 0) {
                count++;
            }
            usleep(1000); 
        }
        
        int status;
        waitpid(pid, &status, 0);
        assert_true(count > 0);
        printf("[Test] Read %d items during concurrency\n", count);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_single_thread_read_write, setup, teardown),
        cmocka_unit_test_setup_teardown(test_multi_process_concurrency, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
