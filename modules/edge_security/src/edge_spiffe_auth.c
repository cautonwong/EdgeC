#include "edge_spiffe_auth.h"
#include <string.h>
#include <stdio.h>

int spiffe_parse_id(const char *uri_str, spiffe_id_t *out_id) {
    if (!uri_str || !out_id) return -1;
    memset(out_id, 0, sizeof(spiffe_id_t));

    const char *prefix = "spiffe://";
    size_t prefix_len = strlen(prefix);
    if (strncmp(uri_str, prefix, prefix_len) != 0) return -2;

    const char *ptr = uri_str + prefix_len;
    /* Format: <trust_domain>/ns/<namespace>/sa/<service_account> */
    char domain[64] = {0};
    char ns[32] = {0};
    char sa[32] = {0};

    if (sscanf(ptr, "%63[^/]/ns/%31[^/]/sa/%31s", domain, ns, sa) == 3) {
        strncpy(out_id->trust_domain, domain, sizeof(out_id->trust_domain) - 1);
        strncpy(out_id->namespace, ns, sizeof(out_id->namespace) - 1);
        strncpy(out_id->service_account, sa, sizeof(out_id->service_account) - 1);
        out_id->valid = true;
        return 0;
    }

    return -3;
}

int spiffe_format_id(const spiffe_id_t *id, char *out_buf, size_t buf_size) {
    if (!id || !out_buf || !id->valid) return -1;
    int len = snprintf(out_buf, buf_size, "spiffe://%s/ns/%s/sa/%s", id->trust_domain, id->namespace, id->service_account);
    if (len < 0 || (size_t)len >= buf_size) return -2;
    return 0;
}

bool spiffe_validate_access(const spiffe_id_t *id, const char *allowed_domain, const char *allowed_ns) {
    if (!id || !id->valid || !allowed_domain || !allowed_ns) return false;
    if (strcmp(id->trust_domain, allowed_domain) != 0) return false;
    if (strcmp(id->namespace, allowed_ns) != 0) return false;
    return true;
}
