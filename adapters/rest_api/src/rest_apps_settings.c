/**
 * @file rest_apps_settings.c
 * @brief App settings admin route.
 */

#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>

#include "edge_service.h"
#include "rest_control_port.h"
#include "rest_internal.h"

char *rest_apps_settings_set(service_t *svc, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) return strdup("{\"status\":\"error\",\"reason\":\"invalid_json\"}");

    cJSON *app_id = cJSON_GetObjectItemCaseSensitive(root, "app_id");
    cJSON *key = cJSON_GetObjectItemCaseSensitive(root, "key");
    cJSON *value = cJSON_GetObjectItemCaseSensitive(root, "value");
    if (!cJSON_IsString(app_id) || !app_id->valuestring[0] || !cJSON_IsString(key) ||
        !key->valuestring[0] || !value) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"missing_fields\"}");
    }
    char *value_json = cJSON_PrintUnformatted(value);
    if (!value_json) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"out_of_memory\"}");
    }

    const rest_control_port_t *control = (const rest_control_port_t *)service_get_control_port(svc);
    char reason[64] = "";
    int rc = (control && control->settings_set)
                 ? control->settings_set(control->ctx, app_id->valuestring, key->valuestring,
                                         value_json, reason, sizeof(reason))
                 : -1;
    free(value_json);
    if (rc == 0) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"ok\"}");
    }

    cJSON *out = cJSON_CreateObject();
    if (!out) {
        cJSON_Delete(root);
        return strdup("{\"status\":\"error\",\"reason\":\"store_error\"}");
    }
    cJSON_AddStringToObject(out, "status", "error");
    cJSON_AddStringToObject(out, "reason", reason[0] ? reason : "store_error");
    cJSON_AddStringToObject(out, "key", key->valuestring);
    char *json = cJSON_PrintUnformatted(out);
    cJSON_Delete(out);
    cJSON_Delete(root);
    return json ? json : strdup("{\"status\":\"error\",\"reason\":\"store_error\"}");
}
