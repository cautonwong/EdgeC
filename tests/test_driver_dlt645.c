#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "shmring.h"

// Mock Contract Table
uint32_t edge_lookup_dlt645(uint32_t di, float *out_scale) {
    if (di == 0x00000000) {
        *out_scale = 1.0f;
        return 1001;
    }
    return 0;
}

extern int dlt645_driver_register(void);

static int setup(void **state) {
    (void)state;
    driver_manager_init();
    dlt645_driver_register();
    return 0;
}

static int teardown(void **state) {
    (void)state;
    driver_manager_fini();
    return 0;
}

static void test_dlt645_batch_cycle(void **state) {
    (void)state;
    edge_driver_t *drv = driver_manager_find("DLT645-2007");
    assert_non_null(drv);

    edge_poll_group_t group = {.slave_id = 123456, .start_addr = 0x00000000};
    uint8_t rx[] = {0x68, 0x56, 0x34, 0x12, 0x00, 0x00, 0x00, 0x68, 0x91,
                    0x08, 0x33, 0x33, 0x33, 0x33, 0x68, 0x39, 0x33, 0x33,
                    0x00,  // CS
                    0x16};
    uint8_t cs = 0;
    for (int i = 0; i < 18; i++) cs += rx[i];
    rx[18] = cs;

    static int found = 0;
    void _on_msg(pipe_msg_t * m, void *u) {
        (void)u;
        if (m->tag_id == 1001) found = 1;
    }

    int ret = drv->parse_batch_res(&group, rx, sizeof(rx), _on_msg, NULL);
    assert_int_equal(ret, 0);
    assert_int_equal(found, 1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dlt645_batch_cycle, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
