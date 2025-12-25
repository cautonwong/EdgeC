#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <cmocka.h>
#include "shmring.h"

#define TEST_RING_NAME "/test_shmring"
#define TEST_SLOT_COUNT 256
#define TEST_WRITE_COUNT 1000

// ============================================================================
// Test Fixture Setup / Teardown
// ============================================================================

static int setup(void **state) {
    shm_ring_t *ring = malloc(sizeof(shm_ring_t));
    assert_non_null(ring);
    
    shm_unlink(TEST_RING_NAME);

    int rv = shmring_create(ring, TEST_RING_NAME, TEST_SLOT_COUNT);
    if (rv != 0) {
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

// ============================================================================
// Basic Tests
// ============================================================================

static void test_create_and_open(void **state) {
    shm_ring_t *ring = (shm_ring_t *)*state;
    assert_int_equal(ring->hdr->magic, SHMRING_MAGIC);
    assert_int_equal(ring->hdr->slot_count, TEST_SLOT_COUNT);

    shm_ring_t ring_opened;
    int rv = shmring_open(&ring_opened, TEST_RING_NAME);
    assert_int_equal(rv, 0);
    assert_int_equal(ring_opened.hdr->magic, SHMRING_MAGIC);
    assert_int_equal(ring_opened.hdr->slot_count, TEST_SLOT_COUNT);

    shmring_close(&ring_opened);
}

static void test_single_thread_read_write(void **state) {
    shm_ring_t *ring = (shm_ring_t *)*state;
    PipeMsg write_msg = {0}, read_msg = {0};
    uint64_t seq = 0;

    assert_int_equal(shm_ring_read(ring, &seq, &read_msg), -1);

    write_msg.ts = 12345;
    write_msg.tag_id = 101;
    write_msg.type = PIPEMSG_SCALAR; 
    write_msg.payload.val_f = 99.9;
    
    shm_ring_write(ring, &write_msg);

    assert_int_equal(shm_ring_read(ring, &seq, &read_msg), 0);
    assert_int_equal(read_msg.ts, write_msg.ts);
    assert_int_equal(read_msg.tag_id, write_msg.tag_id);
    assert_float_equal(read_msg.payload.val_f, write_msg.payload.val_f, 0.001);
}

// ============================================================================
// Multi-Process Test
// ============================================================================

static void writer_process_main() {
    shm_ring_t ring;
    if (shmring_open(&ring, TEST_RING_NAME) != 0) {
        exit(1); 
    }

    PipeMsg msg = {0};
    msg.type = PIPEMSG_SCALAR;
    msg.tag_id = 1001;

    for (int i = 0; i < TEST_WRITE_COUNT; i++) {
        msg.ts = (uint64_t)i; 
        msg.payload.val_i = i;
        shm_ring_write(&ring, &msg);
        usleep(1000); // 1ms delay (Slower than before to help reader)
    }
    shmring_close(&ring);
    exit(0);
}

static void reader_process_main() {
    shm_ring_t ring;
    if (shmring_open(&ring, TEST_RING_NAME) != 0) {
        exit(1);
    }

    PipeMsg msg;
    int reads = 0;
    int64_t last_val = -1;
    uint64_t seq = 0;
    
    // Writer takes ~1000ms total. We poll for 2000ms.
    for (int i = 0; i < 2000; i++) {
        if (shm_ring_read(&ring, &seq, &msg) == 0) {
            if (msg.payload.val_i < last_val) {
                exit(2);
            }
            last_val = msg.payload.val_i;
            reads++;
        }
        usleep(500); // Poll twice as fast as writer
    }
    
    shmring_close(&ring);

    if (reads == 0) exit(3);
    
    exit(0);
}

static void test_multi_process_read_write(void **state) {
    (void) state;
    pid_t writer_pid, reader_pid;
    int writer_status, reader_status;

    writer_pid = fork();
    assert_true(writer_pid >= 0);
    if (writer_pid == 0) {
        writer_process_main();
    }

    reader_pid = fork();
    assert_true(reader_pid >= 0);
    if (reader_pid == 0) {
        reader_process_main();
    }

    waitpid(writer_pid, &writer_status, 0);
    waitpid(reader_pid, &reader_status, 0);

    if (!WIFEXITED(writer_status) || WEXITSTATUS(writer_status) != 0) {
        fail_msg("Writer process failed.");
    }

    if (!WIFEXITED(reader_status)) {
        fail_msg("Reader process crashed.");
    }
    
    if (WEXITSTATUS(reader_status) != 0) {
        switch(WEXITSTATUS(reader_status)) {
            case 1: fail_msg("Reader process failed to open shmring."); break;
            case 2: fail_msg("Reader process read a non-monotonically increasing value."); break;
            case 3: fail_msg("Reader process did not read any values."); break;
            default: fail_msg("Reader process exited with unknown error code: %d", WEXITSTATUS(reader_status)); break;
        }
    }
}


int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_create_and_open, setup, teardown),
        cmocka_unit_test_setup_teardown(test_single_thread_read_write, setup, teardown),
        cmocka_unit_test_setup_teardown(test_multi_process_read_write, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
