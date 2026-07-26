#ifndef EDGE_SPIFFE_AUTH_H
#define EDGE_SPIFFE_AUTH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPIFFE_MAX_STR_LEN 128

typedef struct {
    char trust_domain[64];
    char namespace[32];
    char service_account[32];
    bool valid;
} spiffe_id_t;

int spiffe_parse_id(const char *uri_str, spiffe_id_t *out_id);
int spiffe_format_id(const spiffe_id_t *id, char *out_buf, size_t buf_size);
bool spiffe_validate_access(const spiffe_id_t *id, const char *allowed_domain, const char *allowed_ns);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_SPIFFE_AUTH_H */
