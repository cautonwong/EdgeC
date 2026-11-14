#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <uv.h>

#include "datahub.h"
#include "datahub_shm.h"

static void shadow_path(const char *name, char *out, size_t out_size) {
    snprintf(out, out_size, "/edgeos_shadow_%s", name);
}

static void drain_loop(uv_loop_t *loop) {
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

static void test_init_after_init_preserves_shadow(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    char name[64];
    snprintf(name, sizeof(name), "attach_repro_%d", (int)getpid());

    datahub_storage_t first_storage;
    datahub_t *first = (datahub_t *)&first_storage;
    assert_int_equal(datahub_init(first, &loop, name, 64, 64, NULL, 0, NULL, NULL, NULL), 0);

    pipe_msg_t sentinel;
    pipe_msg_init_scalar(&sentinel, 7, 42.0, 1000);
    assert_int_equal(datahub_write_shadow(first, &sentinel), 0);

    datahub_storage_t second_storage;
    datahub_t *second = (datahub_t *)&second_storage;
    /* A second init must reuse a same-sized live segment, not truncate it. */
    assert_int_equal(datahub_init(second, &loop, name, 64, 64, NULL, 0, NULL, NULL, NULL), 0);

    pipe_msg_t out;
    assert_int_equal(datahub_read_shadow(second, 7, &out), 0);
    assert_float_equal(out.payload.val.f, 42.0, 0.001);

    datahub_destroy(second);
    datahub_destroy(first);

    char path[128];
    shadow_path(name, path, sizeof(path));
    shm_unlink(path);

    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_attach_preserves_existing_shadow(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    char name[64];
    snprintf(name, sizeof(name), "attach_keep_%d", (int)getpid());

    datahub_storage_t owner_storage;
    datahub_t *owner = (datahub_t *)&owner_storage;
    assert_int_equal(datahub_init(owner, &loop, name, 64, 64, NULL, 0, NULL, NULL, NULL), 0);

    pipe_msg_t sentinel;
    pipe_msg_init_scalar(&sentinel, 7, 42.0, 1000);
    assert_int_equal(datahub_write_shadow(owner, &sentinel), 0);

    datahub_storage_t reader_storage;
    datahub_t *reader = (datahub_t *)&reader_storage;
    assert_int_equal(datahub_attach(reader, name), 0);
    assert_true(datahub_shadow_ready(reader));

    struct stat st;
    assert_int_equal(fstat(datahub_get_shadow_fd(reader), &st), 0);
    assert_int_equal(st.st_size, (off_t)(DATAHUB_SHM_HEADER_SIZE + 64 * sizeof(pipe_msg_t)));

    pipe_msg_t out;
    assert_int_equal(datahub_read_shadow(reader, 7, &out), 0);
    assert_float_equal(out.payload.val.f, 42.0, 0.001);

    datahub_destroy(reader);

    pipe_msg_t still_there;
    assert_int_equal(datahub_read_shadow(owner, 7, &still_there), 0);
    assert_float_equal(still_there.payload.val.f, 42.0, 0.001);

    datahub_destroy(owner);

    char path[128];
    shadow_path(name, path, sizeof(path));
    shm_unlink(path);

    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_attach_missing_segment_fails(void **state) {
    (void)state;
    char name[64];
    snprintf(name, sizeof(name), "attach_missing_%d", (int)getpid());

    datahub_storage_t storage;
    datahub_t *hub = (datahub_t *)&storage;
    assert_int_equal(datahub_attach(hub, name), -1);
}


static void test_init_rejects_size_mismatch(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    char name[64];
    snprintf(name, sizeof(name), "attach_size_%d", (int)getpid());

    datahub_storage_t first_storage;
    datahub_t *first = (datahub_t *)&first_storage;
    assert_int_equal(datahub_init(first, &loop, name, 64, 64, NULL, 0, NULL, NULL, NULL), 0);

    datahub_storage_t second_storage;
    datahub_t *second = (datahub_t *)&second_storage;
    assert_int_equal(datahub_init(second, &loop, name, 64, 128, NULL, 0, NULL, NULL, NULL), -1);

    datahub_destroy(first);
    char path[128];
    shadow_path(name, path, sizeof(path));
    shm_unlink(path);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_init_rebuilds_stale_headerless_segment(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    char name[64];
    snprintf(name, sizeof(name), "init_legacy_%d", (int)getpid());
    char path[128];
    shadow_path(name, path, sizeof(path));
    shm_unlink(path);

    int fd = shm_open(path, O_CREAT | O_RDWR, 0600);
    assert_true(fd >= 0);
    off_t size = (off_t)(4 * sizeof(pipe_msg_t));
    assert_int_equal(ftruncate(fd, size), 0);
    pipe_msg_t *map = mmap(NULL, (size_t)size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert_true(map != MAP_FAILED);
    pipe_msg_init_scalar(&map[2], 2, 77.0, 1000);
    munmap(map, (size_t)size);
    close(fd);

    datahub_storage_t storage;
    datahub_t *hub = (datahub_t *)&storage;
    assert_int_equal(datahub_init(hub, &loop, name, 64, 4, NULL, 0, NULL, NULL, NULL), 0);
    struct stat st;
    assert_int_equal(fstat(datahub_get_shadow_fd(hub), &st), 0);
    assert_int_equal(st.st_size, (off_t)(DATAHUB_SHM_HEADER_SIZE + 4 * sizeof(pipe_msg_t)));
    pipe_msg_t out;
    assert_int_equal(datahub_read_shadow(hub, 2, &out), 0);
    assert_int_equal(out.tag_id, 0);
    datahub_destroy(hub);
    shm_unlink(path);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_rebuilt_segment_survives_reopen(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    char name[64];
    snprintf(name, sizeof(name), "legacy_rebuild_%d", (int)getpid());
    char path[128];
    shadow_path(name, path, sizeof(path));
    shm_unlink(path);

    int fd = shm_open(path, O_CREAT | O_RDWR, 0600);
    assert_true(fd >= 0);
    off_t size = (off_t)(4 * sizeof(pipe_msg_t));
    assert_int_equal(ftruncate(fd, size), 0);
    close(fd);

    datahub_storage_t first_storage;
    datahub_t *first = (datahub_t *)&first_storage;
    assert_int_equal(datahub_init(first, &loop, name, 64, 4, NULL, 0, NULL, NULL, NULL), 0);
    struct stat st;
    assert_int_equal(fstat(datahub_get_shadow_fd(first), &st), 0);
    assert_int_equal(st.st_size, (off_t)(DATAHUB_SHM_HEADER_SIZE + 4 * sizeof(pipe_msg_t)));
    datahub_destroy(first);

    datahub_storage_t second_storage;
    datahub_t *second = (datahub_t *)&second_storage;
    assert_int_equal(datahub_init(second, &loop, name, 64, 4, NULL, 0, NULL, NULL, NULL), 0);
    assert_int_equal(fstat(datahub_get_shadow_fd(second), &st), 0);
    assert_int_equal(st.st_size, (off_t)(DATAHUB_SHM_HEADER_SIZE + 4 * sizeof(pipe_msg_t)));
    datahub_destroy(second);
    shm_unlink(path);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_attach_rejects_headerless_segment(void **state) {
    (void)state;
    char name[64];
    snprintf(name, sizeof(name), "attach_legacy_%d", (int)getpid());
    char path[128];
    shadow_path(name, path, sizeof(path));
    shm_unlink(path);

    int fd = shm_open(path, O_CREAT | O_RDWR, 0600);
    assert_true(fd >= 0);
    off_t size = (off_t)(4 * sizeof(pipe_msg_t));
    assert_int_equal(ftruncate(fd, size), 0);
    pipe_msg_t *map = mmap(NULL, (size_t)size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert_true(map != MAP_FAILED);
    pipe_msg_init_scalar(&map[1], 1, 123.0, 1000);
    munmap(map, (size_t)size);
    close(fd);

    datahub_storage_t storage;
    datahub_t *hub = (datahub_t *)&storage;
    assert_int_equal(datahub_attach(hub, name), -1);
    shm_unlink(path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_init_after_init_preserves_shadow),
        cmocka_unit_test(test_init_rebuilds_stale_headerless_segment),
        cmocka_unit_test(test_rebuilt_segment_survives_reopen),
        cmocka_unit_test(test_attach_rejects_headerless_segment),
        cmocka_unit_test(test_attach_preserves_existing_shadow),
        cmocka_unit_test(test_attach_missing_segment_fails),
        cmocka_unit_test(test_init_rejects_size_mismatch),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
