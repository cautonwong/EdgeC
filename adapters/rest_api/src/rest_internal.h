/**
 * @file rest_internal.h
 * @brief Helpers shared by the REST route translation units.
 *
 * Not installed; not part of the public adapter contract.
 */
#ifndef REST_INTERNAL_H
#define REST_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "edge_service.h"

/** Decode an even-length hex payload; caller frees. NULL when invalid. */
uint8_t *rest_hex_decode(const char *hex, size_t *out_len);

/** Install route: verifies and installs a signed `.eapp` package. */
char *rest_apps_package(service_t *svc, const char *body);

/** Upgrade route: verifies a signed `.eapp` and swaps the app package. */
char *rest_apps_upgrade_package(service_t *svc, const char *body);

#endif /* REST_INTERNAL_H */
