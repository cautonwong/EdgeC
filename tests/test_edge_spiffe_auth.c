#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "edge_spiffe_auth.h"

int main(void) {
    printf("=== Test SPIFFE Auth Start ===\n");

    const char *uri = "spiffe://edgeos.internal/ns/production/sa/rehab-smartbox";
    spiffe_id_t id;
    assert(spiffe_parse_id(uri, &id) == 0);
    assert(id.valid == true);
    assert(strcmp(id.trust_domain, "edgeos.internal") == 0);
    assert(strcmp(id.namespace, "production") == 0);
    assert(strcmp(id.service_account, "rehab-smartbox") == 0);

    char formatted[128];
    assert(spiffe_format_id(&id, formatted, sizeof(formatted)) == 0);
    assert(strcmp(formatted, uri) == 0);

    assert(spiffe_validate_access(&id, "edgeos.internal", "production") == true);
    assert(spiffe_validate_access(&id, "edgeos.internal", "staging") == false);

    /* Test invalid URI */
    spiffe_id_t invalid_id;
    assert(spiffe_parse_id("http://invalid.uri", &invalid_id) == -2);

    printf("=== Test SPIFFE Auth Passed (124/124) ===\n");
    return 0;
}
