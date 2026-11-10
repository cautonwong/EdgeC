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
#include <uv.h>

#include "datahub.h"

static void shadow_path(const char *name, char *out, size_t out_size) {
    snprintf(out, out_size, "/edgeos_shadow_%s", name);
}

static void drain_loop(uv_loop_t *loop) {
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

static void test_init_after_init_resets_shadow(void **state) {
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
    /* datahub_init is the create path: it opens with O_TRUNC. This is what the
     * CLI used to do to a live node's shadow. */
    assert_int_equal(datahub_init(second, &loop, name, 64, 256, NULL, 0, NULL, NULL, NULL), 0);

    pipe_msg_t out;
    assert_int_equal(datahub_read_shadow(second, 7, &out), 0);
    assert_float_equal(out.payload.val.f, 0.0, 0.001);

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
    assert_int_equal(st.st_size, (off_t)(64 * sizeof(pipe_msg_t)));

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

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_init_after_init_resets_shadow),
        cmocka_unit_test(test_attach_preserves_existing_shadow),
        cmocka_unit_test(test_attach_missing_segment_fails),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
