/**
 * @file rest_apps_package.c
 * @brief Signed `.eapp` install and upgrade routes.
 */

#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "edge_service.h"
#include "rest_control_port.h"
#include "rest_internal.h"

char *rest_apps_package(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *package_item = cJSON_GetObjectItem(root, "package_hex");
    cJSON *key_item = cJSON_GetObjectItem(root, "idempotency_key");
    cJSON *pubkey_item = cJSON_GetObjectItem(root, "pubkey_hex");
    if (!cJSON_IsString(package_item) || (key_item && !cJSON_IsString(key_item)) ||
        (pubkey_item && !cJSON_IsString(pubkey_item))) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"invalid_payload\"}");
    }

    size_t package_len = 0;
    uint8_t *package = rest_hex_decode(package_item->valuestring, &package_len);
    if (!package) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"invalid_payload\"}");
    }

    uint8_t expected_pubkey[32];
    const uint8_t *expected = NULL;
    if (cJSON_IsString(pubkey_item)) {
        size_t pubkey_len = 0;
        uint8_t *decoded = rest_hex_decode(pubkey_item->valuestring, &pubkey_len);
        if (!decoded || pubkey_len != sizeof(expected_pubkey)) {
            free(decoded);
            free(package);
            cJSON_Delete(root);
            return strdup("{\"status\":\"error\",\"reason\":\"invalid_pubkey\"}");
        }
        memcpy(expected_pubkey, decoded, sizeof(expected_pubkey));
        free(decoded);
        expected = expected_pubkey;
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    if (!control || !control->install_package) {
        free(package);
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"app_manager_unavailable\"}");
    }
    int rc = control->install_package(control->ctx, package, package_len,
                                      cJSON_IsString(key_item) ? key_item->valuestring : NULL,
                                      expected);

    free(package);
    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"package_rejected\"}");
}

char *rest_apps_upgrade_package(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *id_item = cJSON_GetObjectItem(root, "app_id");
    cJSON *package_item = cJSON_GetObjectItem(root, "package_hex");
    cJSON *key_item = cJSON_GetObjectItem(root, "idempotency_key");
    if (!cJSON_IsString(id_item) || !cJSON_IsString(package_item) ||
        (key_item && !cJSON_IsString(key_item))) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"invalid_payload\"}");
    }

    size_t package_len = 0;
    uint8_t *package = rest_hex_decode(package_item->valuestring, &package_len);
    if (!package) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"invalid_payload\"}");
    }

    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    if (!control || !control->upgrade_package) {
        free(package);
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"app_manager_unavailable\"}");
    }
    int rc = control->upgrade_package(control->ctx, id_item->valuestring, package, package_len,
                                      cJSON_IsString(key_item) ? key_item->valuestring : NULL);

    free(package);
    cJSON_Delete(root);
    return rc == 0 ? strdup("{\"status\":\"ok\"}")
                   : strdup("{\"status\":\"error\",\"reason\":\"upgrade_rejected\"}");
}
