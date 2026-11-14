#include "edgeos/app_manifest.h"

#include <cjson/cJSON.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define APP_PRIORITY_MAX 10
#define APP_PIDS_MAX_LIMIT 1024
#define APP_U32_MAX_VALUE 4294967295.0

static int fail(char *reason, size_t size, const char *message) {
    snprintf(reason, size, "%s", message);
    return -1;
}

static int store_string(char *dst, size_t capacity, const char *src, const char *field,
                        char *reason, size_t size) {
    size_t len = strlen(src);
    if (len >= capacity) {
        snprintf(reason, size, "%s is too long", field);
        return -1;
    }
    if (dst) memcpy(dst, src, len + 1);
    return 0;
}

static int valid_id(const char *id) {
    if (!id || !id[0] || strlen(id) >= APP_SPEC_ID_SIZE || !islower((unsigned char)id[0])) return 0;
    for (const char *p = id; *p; p++) {
        if (!(islower((unsigned char)*p) || isdigit((unsigned char)*p) || *p == '-')) return 0;
    }
    return 1;
}

static int valid_version(const char *version) {
    int major = -1, minor = -1, patch = -1;
    char tail = '\0';
    if (!version || strlen(version) >= APP_SPEC_VERSION_SIZE) return 0;
    return sscanf(version, "%d.%d.%d%c", &major, &minor, &patch, &tail) == 3 && major >= 0 &&
           minor >= 0 && patch >= 0;
}

static int known_language(const char *value) {
    static const char *known[] = {"c", "cpp", "rust", "go", "python", "js", NULL};
    for (int i = 0; known[i]; i++) {
        if (strcmp(value, known[i]) == 0) return 1;
    }
    return 0;
}

static int known_profile(const char *value) {
    return strcmp(value, "wasm") == 0 || strcmp(value, "sidecar") == 0 ||
           strcmp(value, "shared") == 0;
}

static int known_arch(const char *value) {
    static const char *known[] = {"any", "aarch64", "x86_64", "armv7", NULL};
    for (int i = 0; known[i]; i++) {
        if (strcmp(value, known[i]) == 0) return 1;
    }
    return 0;
}

static int reject_unknown(cJSON *root, const char *const *known, int count, char *reason,
                          size_t size) {
    for (cJSON *item = root->child; item; item = item->next) {
        int allowed = 0;
        for (int i = 0; i < count; i++) {
            if (strcmp(item->string, known[i]) == 0) {
                allowed = 1;
                break;
            }
        }
        if (!allowed) {
            snprintf(reason, size, "unknown field: %s", item->string);
            return -1;
        }
    }
    return 0;
}

static int read_u32(cJSON *item, uint32_t *out, double minimum, double maximum, const char *message,
                    char *reason, size_t size) {
    double value = cJSON_IsNumber(item) ? item->valuedouble : 0.0;
    if (!cJSON_IsNumber(item) || value < minimum || value > maximum ||
        value != (double)(uint32_t)value) {
        return fail(reason, size, message);
    }
    if (out) *out = (uint32_t)value;
    return 0;
}

static int validate_string_array(cJSON *array, const char *app_id, int publish,
                                 char (*dst)[APP_SPEC_SCOPE_SIZE], size_t *dst_count, char *reason,
                                 size_t size) {
    if (!cJSON_IsArray(array)) return fail(reason, size, "capability scope must be an array");
    size_t count = 0;
    for (cJSON *item = array->child; item; item = item->next) {
        if (!cJSON_IsString(item) || !item->valuestring[0]) {
            return fail(reason, size, "capability scope must be non-empty strings");
        }
        if (strchr(item->valuestring, '*')) return fail(reason, size, "wildcards are not allowed");
        if (publish) {
            char prefix[80];
            snprintf(prefix, sizeof(prefix), "app.%s.", app_id ? app_id : "");
            if (strncmp(item->valuestring, prefix, strlen(prefix)) != 0) {
                return fail(reason, size, "points.publish outside app namespace");
            }
        }
        size_t len = strlen(item->valuestring);
        if (len >= APP_SPEC_SCOPE_SIZE) return fail(reason, size, "capability scope is too long");
        if (count >= (size_t)APP_SPEC_MAX_SCOPES) {
            return fail(reason, size, "too many capability scopes");
        }
        if (dst) memcpy(dst[count], item->valuestring, len + 1);
        count++;
    }
    if (dst_count) *dst_count = count;
    return 0;
}

static int validate_capability_reason(cJSON *capability, const char *name, char *dst, char *reason,
                                      size_t size) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(capability, "reason");
    if (!item) return 0;
    if (!cJSON_IsString(item) || !item->valuestring[0]) {
        snprintf(reason, size, "capabilities.%s.reason must be a non-empty string", name);
        return -1;
    }
    size_t len = strlen(item->valuestring);
    if (len >= APP_SPEC_REASON_SIZE) {
        snprintf(reason, size, "capabilities.%s.reason is too long", name);
        return -1;
    }
    if (dst) memcpy(dst, item->valuestring, len + 1);
    return 0;
}

static int validate_capabilities(cJSON *caps, const char *app_id, app_capabilities_t *out,
                                 char *reason, size_t size) {
    if (!caps) return 0;
    if (!cJSON_IsObject(caps)) return fail(reason, size, "capabilities must be an object");
    const char *known[] = {"tags", "points", "kv", "log"};
    if (reject_unknown(caps, known, 4, reason, size) != 0) return -1;

    cJSON *tags = cJSON_GetObjectItemCaseSensitive(caps, "tags");
    if (tags) {
        if (!cJSON_IsObject(tags)) return fail(reason, size, "capabilities.tags must be an object");
        const char *tag_keys[] = {"read", "write", "reason"};
        if (reject_unknown(tags, tag_keys, 3, reason, size) != 0) return -1;
        cJSON *read = cJSON_GetObjectItemCaseSensitive(tags, "read");
        if (read && validate_string_array(read, app_id, 0, out ? out->read : NULL,
                                          out ? &out->read_count : NULL, reason, size) != 0) {
            return -1;
        }
        cJSON *write = cJSON_GetObjectItemCaseSensitive(tags, "write");
        if (write) {
            if (!cJSON_IsArray(write) || cJSON_GetArraySize(write) != 0) {
                return fail(reason, size, "tags.write is not open in v1");
            }
        }
        if (validate_capability_reason(tags, "tags", out ? out->tags_reason : NULL, reason, size) !=
            0) {
            return -1;
        }
    }

    cJSON *points = cJSON_GetObjectItemCaseSensitive(caps, "points");
    if (points) {
        if (!cJSON_IsObject(points)) {
            return fail(reason, size, "capabilities.points must be an object");
        }
        const char *point_keys[] = {"publish", "reason"};
        if (reject_unknown(points, point_keys, 2, reason, size) != 0) return -1;
        cJSON *publish = cJSON_GetObjectItemCaseSensitive(points, "publish");
        if (publish && validate_string_array(publish, app_id, 1, out ? out->publish : NULL,
                                             out ? &out->publish_count : NULL, reason, size) != 0) {
            return -1;
        }
        if (validate_capability_reason(points, "points", out ? out->points_reason : NULL, reason,
                                       size) != 0) {
            return -1;
        }
    }

    cJSON *kv = cJSON_GetObjectItemCaseSensitive(caps, "kv");
    if (kv) {
        if (!cJSON_IsObject(kv)) return fail(reason, size, "capabilities.kv must be an object");
        if (out) out->kv = 1;
        if (validate_capability_reason(kv, "kv", out ? out->kv_reason : NULL, reason, size) != 0) {
            return -1;
        }
    }

    cJSON *log = cJSON_GetObjectItemCaseSensitive(caps, "log");
    if (log) {
        if (!cJSON_IsBool(log)) return fail(reason, size, "capabilities.log must be boolean");
        if (out) out->log = cJSON_IsTrue(log) ? 1 : 0;
    }
    return 0;
}

static int validate_requires(cJSON *requires, const char *app_id,
                             char (*dst)[APP_SPEC_SCOPE_SIZE], size_t *dst_count, char *reason,
                             size_t size) {
    if (!requires) return 0;
    if (!cJSON_IsObject(requires)) return fail(reason, size, "requires must be an object");
    const char *known[] = {"points"};
    if (reject_unknown(requires, known, 1, reason, size) != 0) return -1;
    cJSON *points = cJSON_GetObjectItemCaseSensitive(requires, "points");
    if (points && validate_string_array(points, app_id, 0, dst, dst_count, reason, size) != 0)
        return -1;
    return 0;
}

/* Every requires.points entry must be exact-matched by a tags.read scope. */
static int check_requires_covered(cJSON *root, char *reason, size_t size) {
    cJSON *requires = cJSON_GetObjectItemCaseSensitive(root, "requires");
    cJSON *points = requires ? cJSON_GetObjectItemCaseSensitive(requires, "points") : NULL;
    if (!points) return 0;
    cJSON *caps = cJSON_GetObjectItemCaseSensitive(root, "capabilities");
    cJSON *tags = caps ? cJSON_GetObjectItemCaseSensitive(caps, "tags") : NULL;
    cJSON *read = tags ? cJSON_GetObjectItemCaseSensitive(tags, "read") : NULL;

    for (cJSON *point = points->child; point; point = point->next) {
        if (!cJSON_IsString(point) || !point->valuestring) continue;
        int covered = 0;
        for (cJSON *scope = read ? read->child : NULL; scope && !covered; scope = scope->next) {
            if (cJSON_IsString(scope) && scope->valuestring &&
                strcmp(scope->valuestring, point->valuestring) == 0)
                covered = 1;
        }
        if (!covered) return fail(reason, size, "requires.points must be covered by tags.read");
    }
    return 0;
}

static int valid_point_name(const char *name) {
    if (!name || !(name[0] >= 'a' && name[0] <= 'z')) return 0;
    size_t length = strlen(name);
    if (length == 0 || length >= APP_SPEC_POINT_NAME_SIZE) return 0;
    for (size_t i = 0; i < length; i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return 0;
    }
    return 1;
}

static int parse_enum_value(cJSON *item, const char *const *names, int count, uint8_t *out,
                            const char *what, char *reason, size_t size) {
    if (!cJSON_IsString(item) || !item->valuestring) {
        snprintf(reason, size, "%s must be a string", what);
        return -1;
    }
    for (int i = 0; i < count; i++) {
        if (strcmp(item->valuestring, names[i]) == 0) {
            if (out) *out = (uint8_t)i;
            return 0;
        }
    }
    snprintf(reason, size, "%s is invalid", what);
    return -1;
}

static int validate_contributes(cJSON *contributes, const char *app_id, app_point_spec_t *out,
                                size_t *out_count, char *reason, size_t size) {
    if (out_count) *out_count = 0;
    if (!contributes) return 0;
    if (!cJSON_IsObject(contributes)) return fail(reason, size, "contributes must be an object");

    cJSON *telemetry = cJSON_GetObjectItemCaseSensitive(contributes, "telemetry");
    if (!telemetry) return 0;
    if (!cJSON_IsObject(telemetry)) {
        return fail(reason, size, "contributes.telemetry must be an object");
    }
    const char *telemetry_keys[] = {"points"};
    if (reject_unknown(telemetry, telemetry_keys, 1, reason, size) != 0) return -1;

    cJSON *points = cJSON_GetObjectItemCaseSensitive(telemetry, "points");
    if (!points) return 0;
    if (!cJSON_IsArray(points)) {
        return fail(reason, size, "contributes.telemetry.points must be an array");
    }

    static const char *types[] = {"number", "integer", "boolean", "text"};
    static const char *retentions[] = {"latest", "latest+history"};
    static const char *stabilities[] = {"stable", "experimental"};
    static const char *sources[] = {"measured", "derived"};
    const char *point_keys[] = {"name",     "type",          "unit",      "contractVersion",
                                "retention", "staleAfterMs", "stability", "source"};

    size_t count = 0;
    for (cJSON *item = points->child; item; item = item->next) {
        if (!cJSON_IsObject(item)) {
            return fail(reason, size, "contributes point must be an object");
        }
        if (reject_unknown(item, point_keys, 8, reason, size) != 0) return -1;
        if (count >= (size_t)APP_SPEC_MAX_POINTS) {
            return fail(reason, size, "too many contributed points (max 256)");
        }

        cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
        if (!cJSON_IsString(name) || !valid_point_name(name->valuestring)) {
            return fail(reason, size, "point name must match [a-z][a-z0-9_]{0,63}");
        }
        for (cJSON *prev = points->child; prev && prev != item; prev = prev->next) {
            cJSON *prev_name = cJSON_GetObjectItemCaseSensitive(prev, "name");
            if (cJSON_IsString(prev_name) && prev_name->valuestring &&
                strcmp(prev_name->valuestring, name->valuestring) == 0) {
                snprintf(reason, size, "duplicate point name: %s", name->valuestring);
                return -1;
            }
        }

        app_point_spec_t point;
        memset(&point, 0, sizeof(point));
        snprintf(point.name, sizeof(point.name), "%s", name->valuestring);
        point.stale_after_ms = APP_SPEC_POINT_STALE_DEFAULT_MS;

        cJSON *type = cJSON_GetObjectItemCaseSensitive(item, "type");
        if (!type) return fail(reason, size, "point type is required");
        if (parse_enum_value(type, types, 4, &point.type, "point type", reason, size) != 0)
            return -1;

        cJSON *contract = cJSON_GetObjectItemCaseSensitive(item, "contractVersion");
        if (!contract || !cJSON_IsNumber(contract) || contract->valuedouble < 1.0 ||
            contract->valuedouble > 65535.0 || contract->valuedouble != (double)(int)contract->valuedouble) {
            return fail(reason, size, "point contractVersion must be an integer between 1 and 65535");
        }
        point.contract_version = (uint16_t)contract->valueint;

        cJSON *unit = cJSON_GetObjectItemCaseSensitive(item, "unit");
        if (unit) {
            if (!cJSON_IsString(unit) || strlen(unit->valuestring) >= APP_SPEC_POINT_UNIT_SIZE) {
                return fail(reason, size, "point unit is too long");
            }
            snprintf(point.unit, sizeof(point.unit), "%s", unit->valuestring);
        }

        cJSON *retention = cJSON_GetObjectItemCaseSensitive(item, "retention");
        if (retention &&
            parse_enum_value(retention, retentions, 2, &point.retention, "point retention", reason,
                             size) != 0) {
            return -1;
        }
        cJSON *stability = cJSON_GetObjectItemCaseSensitive(item, "stability");
        if (stability &&
            parse_enum_value(stability, stabilities, 2, &point.stability, "point stability", reason,
                             size) != 0) {
            return -1;
        }
        cJSON *source = cJSON_GetObjectItemCaseSensitive(item, "source");
        if (source &&
            parse_enum_value(source, sources, 2, &point.source, "point source", reason, size) != 0) {
            return -1;
        }
        cJSON *stale = cJSON_GetObjectItemCaseSensitive(item, "staleAfterMs");
        if (stale) {
            if (!cJSON_IsNumber(stale) || stale->valuedouble < (double)APP_SPEC_POINT_STALE_MIN_MS ||
                stale->valuedouble > (double)APP_SPEC_POINT_STALE_MAX_MS ||
                stale->valuedouble != (double)(int)stale->valuedouble) {
                return fail(reason, size,
                            "point staleAfterMs must be an integer between 100 and 3600000");
            }
            point.stale_after_ms = (uint32_t)stale->valueint;
        }

        if (out) {
            char full[APP_SPEC_POINT_FULL_SIZE];
            int n = snprintf(full, sizeof(full), "app.%s.%s", app_id, point.name);
            if (n < 0 || (size_t)n >= sizeof(full)) {
                return fail(reason, size, "point full name is too long");
            }
            out[count] = point;
        }
        count++;
    }
    if (out_count) *out_count = count;
    return 0;
}

/* Every contributed point must be exact-matched by a points.publish scope. */
static int check_contributes_covered(cJSON *root, char *reason, size_t size) {
    cJSON *contributes = cJSON_GetObjectItemCaseSensitive(root, "contributes");
    cJSON *telemetry = contributes ? cJSON_GetObjectItemCaseSensitive(contributes, "telemetry") : NULL;
    cJSON *points = telemetry ? cJSON_GetObjectItemCaseSensitive(telemetry, "points") : NULL;
    if (!points) return 0;
    cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    cJSON *caps = cJSON_GetObjectItemCaseSensitive(root, "capabilities");
    cJSON *cap_points = caps ? cJSON_GetObjectItemCaseSensitive(caps, "points") : NULL;
    cJSON *publish = cap_points ? cJSON_GetObjectItemCaseSensitive(cap_points, "publish") : NULL;

    for (cJSON *point = points->child; point; point = point->next) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(point, "name");
        if (!cJSON_IsString(name) || !name->valuestring || !id || !id->valuestring) continue;
        char full[APP_SPEC_POINT_FULL_SIZE];
        int n = snprintf(full, sizeof(full), "app.%s.%s", id->valuestring, name->valuestring);
        if (n < 0 || (size_t)n >= sizeof(full)) {
            return fail(reason, size, "point full name is too long");
        }
        int covered = 0;
        for (cJSON *scope = publish ? publish->child : NULL; scope && !covered;
             scope = scope->next) {
            if (cJSON_IsString(scope) && scope->valuestring &&
                strcmp(scope->valuestring, full) == 0) {
                covered = 1;
            }
        }
        if (!covered) {
            snprintf(reason, size, "contributed point %s must be covered by points.publish", full);
            return -1;
        }
    }
    return 0;
}

static int known_preview(const char *value) {    static const char *known[] = {"ui.pages", "app.functions", "settings", NULL};
    for (int i = 0; known[i]; i++) {
        if (strcmp(value, known[i]) == 0) return 1;
    }
    return 0;
}

static int validate_preview(cJSON *preview, const app_manifest_policy_t *policy, app_spec_t *out,
                            char *reason, size_t size) {
    if (!preview) return 0;
    if (!cJSON_IsArray(preview)) return fail(reason, size, "preview must be an array");
    size_t count = 0;
    for (cJSON *item = preview->child; item; item = item->next) {
        if (!cJSON_IsString(item) || !item->valuestring[0]) {
            return fail(reason, size, "preview must contain non-empty strings");
        }
        if (!known_preview(item->valuestring)) {
            snprintf(reason, size, "unknown preview: %s", item->valuestring);
            return -1;
        }
        if (!policy || !policy->allow_preview_apps) {
            snprintf(reason, size, "DENIED_PREVIEW: %s", item->valuestring);
            return -1;
        }
        if (count >= (size_t)APP_SPEC_MAX_PREVIEWS) {
            return fail(reason, size, "too many preview features");
        }
        if (store_string(out ? out->preview[count] : NULL, APP_SPEC_PREVIEW_SIZE, item->valuestring,
                         "preview", reason, size) != 0) {
            return -1;
        }
        count++;
    }
    if (out) out->preview_count = count;
    return 0;
}

static int validate_limits(cJSON *limits, app_limits_t *out, char *reason, size_t size) {
    if (!cJSON_IsObject(limits)) return fail(reason, size, "limits must be an object");
    if (read_u32(cJSON_GetObjectItemCaseSensitive(limits, "heapKb"), out ? &out->heap_kb : NULL,
                 1.0, APP_U32_MAX_VALUE, "limits.heapKb must be a positive integer", reason,
                 size) != 0) {
        return -1;
    }
    if (read_u32(cJSON_GetObjectItemCaseSensitive(limits, "storageKb"),
                 out ? &out->storage_kb : NULL, 1.0, APP_U32_MAX_VALUE,
                 "limits.storageKb must be a positive integer", reason, size) != 0) {
        return -1;
    }
    if (read_u32(cJSON_GetObjectItemCaseSensitive(limits, "cpuMsPerSec"),
                 out ? &out->cpu_ms_per_sec : NULL, 1.0, APP_U32_MAX_VALUE,
                 "limits.cpuMsPerSec must be a positive integer", reason, size) != 0) {
        return -1;
    }
    if (read_u32(cJSON_GetObjectItemCaseSensitive(limits, "callsPerSec"),
                 out ? &out->calls_per_sec : NULL, 1.0, APP_U32_MAX_VALUE,
                 "limits.callsPerSec must be a positive integer", reason, size) != 0) {
        return -1;
    }
    if (read_u32(cJSON_GetObjectItemCaseSensitive(limits, "publishPerSec"),
                 out ? &out->publish_per_sec : NULL, 1.0, APP_U32_MAX_VALUE,
                 "limits.publishPerSec must be a positive integer", reason, size) != 0) {
        return -1;
    }
    cJSON *pids = cJSON_GetObjectItemCaseSensitive(limits, "pidsMax");
    if (pids) {
        if (read_u32(pids, out ? &out->pids_max : NULL, 1.0, (double)APP_PIDS_MAX_LIMIT,
                     "limits.pidsMax must be an integer between 1 and 1024", reason, size) != 0) {
            return -1;
        }
    }
    return 0;
}

static void spec_defaults(app_spec_t *spec) {
    memset(spec, 0, sizeof(*spec));
    snprintf(spec->arch, sizeof(spec->arch), "%s", APP_SPEC_DEFAULT_ARCH);
    snprintf(spec->license, sizeof(spec->license), "%s", APP_SPEC_DEFAULT_LICENSE);
    spec->limits.pids_max = APP_SPEC_DEFAULT_PIDS_MAX;
    spec->priority = APP_SPEC_DEFAULT_PRIORITY;
}

int edge_app_manifest_parse_with_policy(const char *json, const app_manifest_policy_t *policy,
                                        app_spec_t *out, char *reason, size_t reason_size) {
    char dummy[1];
    if (!reason || reason_size == 0) {
        reason = dummy;
        reason_size = sizeof(dummy);
    }
    reason[0] = '\0';
    if (!json) return fail(reason, reason_size, "manifest is empty");
    cJSON *root = cJSON_Parse(json);
    if (!root) return fail(reason, reason_size, "manifest is not valid JSON");
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return fail(reason, reason_size, "manifest must be an object");
    }

    static const char *const known[] = {
        "abi",         "sdk",          "minHost",      "id",
        "version",     "publisher_id", "language",     "execution_profile",
        "entry",       "limits",       "capabilities", "runtime_engine",
        "name",        "description",  "products",     "settings",
        "contributes", "requires",     "fault_policy", "tick_ms",
        "python",      "javascript",   "diagnostics",  "arch",
        "license",     "resources",    "preview"};
    int rc =
        reject_unknown(root, known, (int)(sizeof(known) / sizeof(known[0])), reason, reason_size);

    if (out) spec_defaults(out);

    cJSON *abi = cJSON_GetObjectItemCaseSensitive(root, "abi");
    if (rc == 0) {
        if (!cJSON_IsNumber(abi) || abi->valuedouble != 1.0) {
            rc = fail(reason, reason_size, "abi must be 1");
        } else if (out) {
            out->abi = 1;
        }
    }
    cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    if (rc == 0 && (!cJSON_IsString(id) || !valid_id(id->valuestring))) {
        rc = fail(reason, reason_size, "id must match [a-z][a-z0-9-]{0,62}");
    } else if (rc == 0 && out) {
        rc = store_string(out->id, APP_SPEC_ID_SIZE, id->valuestring, "id", reason, reason_size);
    }
    cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
    if (rc == 0 && (!cJSON_IsString(version) || !valid_version(version->valuestring))) {
        rc = fail(reason, reason_size, "version must be semver");
    } else if (rc == 0 && out) {
        rc = store_string(out->version, APP_SPEC_VERSION_SIZE, version->valuestring, "version",
                          reason, reason_size);
    }
    cJSON *publisher = cJSON_GetObjectItemCaseSensitive(root, "publisher_id");
    if (rc == 0 && publisher) {
        if (!cJSON_IsString(publisher) || !publisher->valuestring[0]) {
            rc = fail(reason, reason_size, "publisher_id must be a non-empty string");
        } else {
            rc = store_string(out ? out->publisher_id : NULL, APP_SPEC_PUBLISHER_ID_SIZE,
                              publisher->valuestring, "publisher_id", reason, reason_size);
        }
    }
    cJSON *language = cJSON_GetObjectItemCaseSensitive(root, "language");
    if (rc == 0 && (!cJSON_IsString(language) || !known_language(language->valuestring))) {
        rc = fail(reason, reason_size, "language is not supported");
    } else if (rc == 0 && out) {
        rc = store_string(out->language, APP_SPEC_LANGUAGE_SIZE, language->valuestring, "language",
                          reason, reason_size);
    }
    cJSON *profile = cJSON_GetObjectItemCaseSensitive(root, "execution_profile");
    if (rc == 0 && (!cJSON_IsString(profile) || !known_profile(profile->valuestring))) {
        rc = fail(reason, reason_size, "execution_profile is invalid");
    } else if (rc == 0 && out) {
        rc = store_string(out->execution_profile, APP_SPEC_PROFILE_SIZE, profile->valuestring,
                          "execution_profile", reason, reason_size);
    }
    cJSON *entry = cJSON_GetObjectItemCaseSensitive(root, "entry");
    if (rc == 0 &&
        (!cJSON_IsString(entry) || !entry->valuestring[0] || entry->valuestring[0] == '/')) {
        rc = fail(reason, reason_size, "entry must be a relative package path");
    } else if (rc == 0 && out) {
        rc = store_string(out->entry, APP_SPEC_ENTRY_SIZE, entry->valuestring, "entry", reason,
                          reason_size);
    }
    if (rc == 0) {
        rc = validate_limits(cJSON_GetObjectItemCaseSensitive(root, "limits"),
                             out ? &out->limits : NULL, reason, reason_size);
    }
    if (rc == 0) {
        rc = validate_capabilities(cJSON_GetObjectItemCaseSensitive(root, "capabilities"),
                                   id ? id->valuestring : "", out ? &out->capabilities : NULL,
                                   reason, reason_size);
    }
    if (rc == 0) {
        rc = validate_requires(cJSON_GetObjectItemCaseSensitive(root, "requires"),
                               id ? id->valuestring : "",
                               out ? out->requires_points : NULL,
                               out ? &out->requires_points_count : NULL, reason, reason_size);
    }
    if (rc == 0) rc = check_requires_covered(root, reason, reason_size);
    if (rc == 0) {
        rc = validate_contributes(cJSON_GetObjectItemCaseSensitive(root, "contributes"),
                                  id ? id->valuestring : "", out ? out->points : NULL,
                                  out ? &out->points_count : NULL, reason, reason_size);
    }
    if (rc == 0) rc = check_contributes_covered(root, reason, reason_size);

    cJSON *arch = cJSON_GetObjectItemCaseSensitive(root, "arch");
    if (rc == 0 && arch) {
        if (!cJSON_IsString(arch) || !known_arch(arch->valuestring)) {
            rc = fail(reason, reason_size, "arch is invalid");
        } else {
            rc = store_string(out ? out->arch : NULL, APP_SPEC_ARCH_SIZE, arch->valuestring, "arch",
                              reason, reason_size);
        }
    }
    cJSON *license = cJSON_GetObjectItemCaseSensitive(root, "license");
    if (rc == 0 && license) {
        if (!cJSON_IsString(license) || !license->valuestring[0]) {
            rc = fail(reason, reason_size, "license must be a non-empty string");
        } else {
            rc = store_string(out ? out->license : NULL, APP_SPEC_LICENSE_SIZE,
                              license->valuestring, "license", reason, reason_size);
        }
    }
    if (rc == 0) {
        rc = validate_preview(cJSON_GetObjectItemCaseSensitive(root, "preview"), policy, out,
                              reason, reason_size);
    }
    cJSON *resources = cJSON_GetObjectItemCaseSensitive(root, "resources");
    if (rc == 0 && resources) {
        if (!cJSON_IsObject(resources)) {
            rc = fail(reason, reason_size, "resources must be an object");
        } else {
            const char *resource_keys[] = {"priority"};
            if (reject_unknown(resources, resource_keys, 1, reason, reason_size) != 0) {
                rc = -1;
            } else {
                cJSON *priority = cJSON_GetObjectItemCaseSensitive(resources, "priority");
                if (priority) {
                    rc = read_u32(priority, out ? &out->priority : NULL, 0.0,
                                  (double)APP_PRIORITY_MAX,
                                  "resources.priority must be an integer between 0 and 10", reason,
                                  reason_size);
                }
            }
        }
    }

    cJSON_Delete(root);
    if (rc != 0 && out) memset(out, 0, sizeof(*out));
    return rc;
}

int edge_app_manifest_parse(const char *json, app_spec_t *out, char *reason, size_t reason_size) {
    return edge_app_manifest_parse_with_policy(json, NULL, out, reason, reason_size);
}

int edge_app_manifest_validate_json(const char *json, char *reason, size_t reason_size) {
    return edge_app_manifest_parse_with_policy(json, NULL, NULL, reason, reason_size);
}

int edge_app_manifest_validate_json_with_policy(const char *json,
                                                const app_manifest_policy_t *policy, char *reason,
                                                size_t reason_size) {
    return edge_app_manifest_parse_with_policy(json, policy, NULL, reason, reason_size);
}
