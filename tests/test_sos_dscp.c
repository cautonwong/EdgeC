#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include "edge_sos_dscp.h"

static void test_sos_dscp_emergency_tagger(void** state) {
    (void)state;

    // 1. Verify emergency tag detection
    assert_true(edge_sos_is_emergency_event(12001, 1.0)); // Emergency SOS button press
    assert_true(edge_sos_is_emergency_event(12002, 0.90)); // Fall detected
    assert_false(edge_sos_is_emergency_event(10001, 220.0)); // Normal voltage

    // 2. Verify DSCP Expedited Forwarding socket option setting
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    assert_true(sockfd >= 0);

    int rc = edge_sos_set_socket_dscp(sockfd);
    assert_int_equal(rc, 0);

    close(sockfd);

    printf("🔥 [SUCCESS] SOS DSCP Expedited Forwarding priority tagger test passed!\n");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sos_dscp_emergency_tagger),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
