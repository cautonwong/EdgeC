/**
 * @file test_datahub_shm_legacy.c
 * @brief Rebuild a headerless shadow segment under the current header.
 */

#include <cmocka.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "datahub_priv.h"
#include "datahub_shm.h"

static void test_legacy_shadow_rejected(void **state) {
    (void)state;
    const char *shm = "/edgeos_shadow_test_legacy";
    const char *lock = "/edgeos_shadow_test_legacy.lock";
    shm_unlink(shm);
    shm_unlink(lock);

    const uint32_t capacity = 4;
    int fd = shm_open(shm, O_RDWR | O_CREAT | O_EXCL, 0666);
    assert_true(fd >= 0);
    assert_int_equal(ftruncate(fd, (off_t)(capacity * sizeof(pipe_msg_t))), 0);
    close(fd);

    datahub_t hub;
    memset(&hub, 0, sizeof(hub));
    datahub_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.backend_type = DATAHUB_BACKEND_SHM;
    snprintf(cfg.shm_name, sizeof(cfg.shm_name), "test_legacy");
    cfg.shadow_capacity = capacity;
    cfg.ring_slots = 8;
    /* A headerless segment must be rejected: consumers mapped it at offset 0,
     * so rebuilding in place would corrupt the new header. */
    assert_int_equal(datahub_shm_init(&hub, &cfg), -1);

    /* Explicit cleanup is the documented migration step. */
    assert_int_equal(shm_unlink(shm), 0);
    memset(&hub, 0, sizeof(hub));
    assert_int_equal(datahub_shm_init(&hub, &cfg), 0);

    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.tag_id = 3;
    msg.payload.val.f = 1.5f;
    assert_int_equal(datahub_shm_write_shadow(&hub, &msg), 0);

    pipe_msg_t out;
    memset(&out, 0, sizeof(out));
    assert_int_equal(datahub_shm_read_shadow(&hub, 3, &out), 0);
    assert_int_equal(out.tag_id, 3);
    assert_true(out.payload.val.f == 1.5f);

    int after_fd = shm_open(shm, O_RDONLY, 0);
    assert_true(after_fd >= 0);
    struct stat after;
    assert_int_equal(fstat(after_fd, &after), 0);
    close(after_fd);
    assert_true((size_t)after.st_size > capacity * sizeof(pipe_msg_t));

    datahub_shm_destroy(&hub);

    shm_unlink(shm);
    shm_unlink(lock);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_legacy_shadow_rejected),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
