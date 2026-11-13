#include <cmocka.h>
#include "thread_port_uv.h"
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>

#include "northbound_mgr.h"
#include "north_driver_test_port.h"

static void test_restart_reattaches_driver(void **state) {
    (void)state;
    northbound_mgr_t mgr;
    nb_source_ops_t source = {0};
    nb_transport_ops_t transport = {0};
    assert_int_equal(northbound_mgr_init_ex(&mgr, source, "restart", NULL, transport), 0);
    northbound_mgr_set_driver_port(&mgr, test_north_driver_port());

    assert_int_equal(northbound_mgr_start(&mgr), 0);
    usleep(100000);
    assert_non_null(mgr.nb_driver);
    northbound_mgr_stop(&mgr);
    assert_null(mgr.nb_driver);

    assert_int_equal(northbound_mgr_start(&mgr), 0);
    usleep(100000);
    assert_non_null(mgr.nb_driver);
    northbound_mgr_stop(&mgr);
    assert_null(mgr.nb_driver);

    northbound_mgr_destroy(&mgr);
}

int main(void) {
    edge_thread_port_install_uv();
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_restart_reattaches_driver),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
