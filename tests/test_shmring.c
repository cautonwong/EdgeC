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
    ShmRing *ring = malloc(sizeof(ShmRing));
    assert_non_null(ring);
    
    // Clean up any previous failed tests
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
    ShmRing *ring = (ShmRing *)*state;
    shmring_destroy(ring);
    free(ring);
    return 0;
}

// ============================================================================
// Basic Tests
// ============================================================================

static void test_create_and_open(void **state) {
    ShmRing *ring = (ShmRing *)*state;
    assert_int_equal(ring->hdr->magic, SHMRING_MAGIC);
    assert_int_equal(ring->hdr->slot_count, TEST_SLOT_COUNT);

    ShmRing ring_opened;
    int rv = shmring_open(&ring_opened, TEST_RING_NAME);
    assert_int_equal(rv, 0);
    assert_int_equal(ring_opened.hdr->magic, SHMRING_MAGIC);
    assert_int_equal(ring_opened.hdr->slot_count, TEST_SLOT_COUNT);

    shmring_close(&ring_opened);
}

static void test_single_thread_read_write(void **state) {
    ShmRing *ring = (ShmRing *)*state;
    PipeMsg write_msg = {0}, read_msg = {0};

    // Test initial read fails
    assert_false(shmring_read_latest(ring, &read_msg));

    // Write first message
    write_msg.ts = 12345;
    write_msg.tag_id = 101;
    write_msg.type = PIPEMSG_SCALAR;
    write_msg.payload.val_f = 99.9;
    
    shmring_write(ring, &write_msg);

    // Read it back
    assert_true(shmring_read_latest(ring, &read_msg));
    assert_int_equal(read_msg.ts, write_msg.ts);
    assert_int_equal(read_msg.tag_id, write_msg.tag_id);
    assert_float_equal(read_msg.payload.val_f, write_msg.payload.val_f, 0.001);
}

// ============================================================================
// Multi-Process Test
// ============================================================================

static void writer_process_main() {
    ShmRing ring;
    if (shmring_open(&ring, TEST_RING_NAME) != 0) {
        exit(1); // Fail parent test
    }

    PipeMsg msg = {0};
    msg.type = PIPEMSG_SCALAR;
    msg.tag_id = 1001;

    for (int i = 0; i < TEST_WRITE_COUNT; i++) {
        msg.ts = i; // Use 'i' as a sequence number
        msg.payload.val_i = i;
        shmring_write(&ring, &msg);
        usleep(100); // 100us delay
    }
    shmring_close(&ring);
    exit(0);
}

static void reader_process_main() {
    ShmRing ring;
    if (shmring_open(&ring, TEST_RING_NAME) != 0) {
        exit(1);
    }

    PipeMsg msg;
    int reads = 0;
    int64_t last_val = -1;
    
    // Try to read for a certain duration
    for (int i = 0; i < 200; i++) {
        if (shmring_read_latest(&ring, &msg)) {
            // Check that value is monotonically increasing
            if (msg.payload.val_i < last_val) {
                // This would indicate a read of stale data, which seqlock should prevent
                exit(2);
            }
            last_val = msg.payload.val_i;
            reads++;
        }
        usleep(1000); // Read slower than writer
    }
    
    shmring_close(&ring);

    // Check if we read anything at all
    if (reads == 0) exit(3);
    // Check if we read a value close to the end
    if (last_val < (TEST_WRITE_COUNT - 100)) exit(4);
    
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

    // Wait for both children to complete
    waitpid(writer_pid, &writer_status, 0);
    waitpid(reader_pid, &reader_status, 0);

    // Check that child processes exited cleanly
    assert_true(WIFEXITED(writer_status));
    assert_int_equal(WEXITSTATUS(writer_status), 0);

    assert_true(WIFEXITED(reader_status));
    if (WEXITSTATUS(reader_status) != 0) {
        switch(WEXITSTATUS(reader_status)) {
            case 1: fail_msg("Reader process failed to open shmring."); break;
            case 2: fail_msg("Reader process read a non-monotonically increasing value."); break;
            case 3: fail_msg("Reader process did not read any values."); break;
            case 4: fail_msg("Reader process did not read a recent value from the writer."); break;
            default: fail_msg("Reader process exited with unknown error code."); break;
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
