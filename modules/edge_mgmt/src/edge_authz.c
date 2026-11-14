/**
 * @file edge_authz.c
 * @brief Grant validation and runtime authorization checks.
 */

#include "edgeos/edge_authz.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <string.h>

const char *edge_authz_result_code(edge_authz_result_t result) {
    switch (result) {
    case EDGE_AUTHZ_OK:
        return "ok";
    case EDGE_AUTHZ_DENIED_NOT_GRANTED:
        return "denied_not_granted";
    case EDGE_AUTHZ_DENIED_EXPIRED:
        return "denied_expired";
    case EDGE_AUTHZ_DENIED_POLICY:
        return "denied_policy";
    case EDGE_AUTHZ_DENIED_SCOPE:
        return "denied_scope";
    case EDGE_AUTHZ_DENIED_QUOTA:
        return "denied_quota";
    case EDGE_AUTHZ_DENIED_REVOKED:
        return "denied_revoked";
    case EDGE_AUTHZ_DENIED_PACKAGE_MISMATCH:
        return "denied_package_mismatch";
    case EDGE_AUTHZ_INVALID_MANIFEST:
        return "invalid_manifest";
    case EDGE_AUTHZ_STORE_ERROR:
        return "store_error";
    default:
        return "denied_policy";
    }
}

static void copy_text(char *dst, size_t capacity, const char *src) {
    snprintf(dst, capacity, "%s", src ? src : "");
}

static bool scope_covered(const char *scope_json, const char *scope) {
    cJSON *root = cJSON_Parse(scope_json);
    if (!root) return strcmp(scope_json, scope) == 0;

    bool found = false;
    if (cJSON_IsArray(root)) {
        for (cJSON *item = root->child; item && !found; item = item->next) {
            if (cJSON_IsString(item) && item->valuestring &&
                strcmp(item->valuestring, scope) == 0)
                found = true;
        }
    } else if (cJSON_IsString(root) && root->valuestring) {
        found = strcmp(root->valuestring, scope) == 0;
    }
    cJSON_Delete(root);
    return found;
}

edge_authz_result_t edge_authz_validate_grant(const edge_app_grant_t *grant) {
    if (!grant) return EDGE_AUTHZ_INVALID_MANIFEST;
    if (grant->app_id[0] == '\0' || grant->capability[0] == '\0')
        return EDGE_AUTHZ_INVALID_MANIFEST;
    if (grant->sensitivity_max >= EDGE_AUTHZ_SENSITIVITY_SENSITIVE && grant->purpose[0] == '\0')
        return EDGE_AUTHZ_INVALID_MANIFEST;
    return EDGE_AUTHZ_OK;
}

edge_authz_result_t edge_authz_check(const edge_authz_engine_t *engine, const char *app_id,
                                     const char *capability, const char *scope, uint64_t now) {
    if (!engine || !engine->repo || !engine->repo->get_grant || !app_id || !capability || !scope)
        return EDGE_AUTHZ_DENIED_NOT_GRANTED;
    if (!engine->enforce) return EDGE_AUTHZ_OK;

    const edge_authz_repo_t *repo = engine->repo;
    edge_app_grant_t grant;
    if (repo->get_grant(repo->ctx, app_id, capability, &grant) != 1)
        return EDGE_AUTHZ_DENIED_NOT_GRANTED;
    if (grant.state != EDGE_GRANT_GRANTED) return EDGE_AUTHZ_DENIED_NOT_GRANTED;
    if (grant.revoked_at != 0) return EDGE_AUTHZ_DENIED_REVOKED;
    if (grant.expires_at != 0 && now >= grant.expires_at) return EDGE_AUTHZ_DENIED_EXPIRED;
    if (edge_authz_validate_grant(&grant) != EDGE_AUTHZ_OK) return EDGE_AUTHZ_DENIED_POLICY;
    if (!scope_covered(grant.scope_json, scope)) return EDGE_AUTHZ_DENIED_SCOPE;

    uint8_t sensitivity = 0;
    edge_point_profile_t profile;
    if (repo->get_point_profile &&
        repo->get_point_profile(repo->ctx, scope, &profile) == 1)
        sensitivity = profile.sensitivity;
    if (sensitivity > grant.sensitivity_max) return EDGE_AUTHZ_DENIED_POLICY;

    /* 8.16 restricted mode: the publisher's level caps the point sensitivity
     * an app can ever reach. L0 has no row and is capped at 0; an unverified
     * publisher is capped at 1 even when its configured level is higher. */
    uint8_t cap = 0;
    if (repo->get_publisher_trust) {
        edge_publisher_trust_t trust;
        memset(&trust, 0, sizeof(trust));
        if (repo->get_publisher_trust(repo->ctx, app_id, &trust) == 1) {
            cap = trust.verified ? trust.trust_level : (trust.trust_level > 1 ? 1 : trust.trust_level);
        }
    }
    if (sensitivity > cap) return EDGE_AUTHZ_DENIED_POLICY;
    return EDGE_AUTHZ_OK;
}

edge_authz_result_t edge_authz_check_package(const char *installed_digest,
                                             const char *runtime_digest) {
    if (!installed_digest || !runtime_digest) return EDGE_AUTHZ_DENIED_PACKAGE_MISMATCH;
    if (installed_digest[0] == '\0' || strcmp(installed_digest, runtime_digest) != 0)
        return EDGE_AUTHZ_DENIED_PACKAGE_MISMATCH;
    return EDGE_AUTHZ_OK;
}

void edge_authz_engine_init(edge_authz_engine_t *engine, const edge_authz_repo_t *repo) {
    if (!engine) return;
    engine->repo = repo;
    engine->enforce = false;
}

void edge_authz_engine_set_enforcement(edge_authz_engine_t *engine, bool enabled) {
    if (engine) engine->enforce = enabled;
}

/* Returns 1 when every requested scope is covered, 0 when one is new, -1 when
 * the requested list is not a JSON array/string of strings. */
static int requested_scopes_subset(const char *requested_scope_json, const char *granted) {
    cJSON *root = cJSON_Parse(requested_scope_json);
    if (!root) return -1;
    int result = 1;
    if (cJSON_IsArray(root)) {
        for (cJSON *item = root->child; item && result == 1; item = item->next) {
            if (!cJSON_IsString(item) || !item->valuestring)
                result = -1;
            else if (!scope_covered(granted, item->valuestring))
                result = 0;
        }
    } else if (cJSON_IsString(root) && root->valuestring) {
        result = scope_covered(granted, root->valuestring) ? 1 : 0;
    } else {
        result = -1;
    }
    cJSON_Delete(root);
    return result;
}

edge_authz_inherit_t edge_authz_inherit_grant(const edge_app_grant_t *existing,
                                              const char *requested_scope_json,
                                              edge_app_grant_t *out) {
    if (!existing || !requested_scope_json || !out) return EDGE_AUTHZ_INHERIT_INVALID;
    *out = *existing;
    if (existing->revoked_at != 0) return EDGE_AUTHZ_INHERIT_REVOKED;

    int subset = requested_scopes_subset(requested_scope_json, existing->scope_json);
    if (subset < 0) return EDGE_AUTHZ_INHERIT_INVALID;

    copy_text(out->scope_json, sizeof(out->scope_json), requested_scope_json);
    if (subset == 0) {
        if (existing->state != EDGE_GRANT_GRANTED) return EDGE_AUTHZ_INHERIT_UNCHANGED;
        out->state = EDGE_GRANT_PENDING;
        return EDGE_AUTHZ_INHERIT_AWAITING;
    }
    if (existing->state != EDGE_GRANT_GRANTED) return EDGE_AUTHZ_INHERIT_UNCHANGED;
    return strcmp(existing->scope_json, requested_scope_json) == 0 ? EDGE_AUTHZ_INHERIT_UNCHANGED
                                                                  : EDGE_AUTHZ_INHERIT_NARROWED;
}

edge_authz_result_t edge_authz_approve_grant(const edge_authz_store_t *store,
                                             edge_app_grant_t *grant, uint64_t now) {
    if (!store || !store->save_grant || !grant) return EDGE_AUTHZ_STORE_ERROR;
    if (edge_authz_validate_grant(grant) != EDGE_AUTHZ_OK) return EDGE_AUTHZ_INVALID_MANIFEST;

    grant->state = EDGE_GRANT_GRANTED;
    grant->granted_at = now;
    grant->revoked_at = 0;
    return store->save_grant(store->ctx, grant) == 0 ? EDGE_AUTHZ_OK : EDGE_AUTHZ_STORE_ERROR;
}

edge_authz_result_t edge_authz_revoke_grant(const edge_authz_store_t *store, const char *app_id,
                                            const char *capability, uint64_t now) {
    if (!store || !store->get_grant || !store->save_grant || !app_id || !capability)
        return EDGE_AUTHZ_STORE_ERROR;

    edge_app_grant_t grant;
    int rc = store->get_grant(store->ctx, app_id, capability, &grant);
    if (rc == 0) return EDGE_AUTHZ_DENIED_NOT_GRANTED;
    if (rc < 0) return EDGE_AUTHZ_STORE_ERROR;
    if (grant.revoked_at != 0) return EDGE_AUTHZ_OK; /* idempotent, keep first timestamp */
    grant.revoked_at = now != 0 ? now : 1; /* 0 means not revoked */
    return store->save_grant(store->ctx, &grant) == 0 ? EDGE_AUTHZ_OK : EDGE_AUTHZ_STORE_ERROR;
}

edge_authz_inherit_t edge_authz_inherit_upgrade(const edge_authz_store_t *store,
                                                const char *app_id, const char *capability,
                                                const char *requested_scope_json, uint64_t now) {
    (void)now;
    if (!store || !store->get_grant || !store->save_grant || !app_id || !capability ||
        !requested_scope_json)
        return EDGE_AUTHZ_INHERIT_STORE_ERROR;

    edge_app_grant_t existing;
    int rc = store->get_grant(store->ctx, app_id, capability, &existing);
    if (rc < 0) return EDGE_AUTHZ_INHERIT_STORE_ERROR;

    int empty = requested_scopes_subset(requested_scope_json, "[]");
    if (empty < 0) return EDGE_AUTHZ_INHERIT_INVALID;
    if (rc == 0 && empty == 1) return EDGE_AUTHZ_INHERIT_UNCHANGED;

    edge_app_grant_t out;
    edge_authz_inherit_t result;
    if (rc == 0) {
        memset(&out, 0, sizeof(out));
        copy_text(out.app_id, sizeof(out.app_id), app_id);
        copy_text(out.capability, sizeof(out.capability), capability);
        copy_text(out.scope_json, sizeof(out.scope_json), requested_scope_json);
        out.state = EDGE_GRANT_PENDING;
        result = EDGE_AUTHZ_INHERIT_AWAITING;
    } else {
        result = edge_authz_inherit_grant(&existing, requested_scope_json, &out);
        if (result == EDGE_AUTHZ_INHERIT_INVALID || result == EDGE_AUTHZ_INHERIT_REVOKED)
            return result;
    }
    if (store->save_grant(store->ctx, &out) != 0) return EDGE_AUTHZ_INHERIT_STORE_ERROR;
    return result;
}

/* --- In-memory repository --- */

void edge_authz_memory_init(edge_authz_memory_store_t *store) {
    if (store) memset(store, 0, sizeof(*store));
}

int edge_authz_memory_add_grant(edge_authz_memory_store_t *store,
                                const edge_app_grant_t *grant) {
    if (!store || !grant) return -1;
    for (size_t i = 0; i < store->grant_count; i++) {
        if (strcmp(store->grants[i].app_id, grant->app_id) == 0 &&
            strcmp(store->grants[i].capability, grant->capability) == 0) {
            store->grants[i] = *grant;
            return 0;
        }
    }
    if (store->grant_count >= EDGE_AUTHZ_MEMORY_MAX_GRANTS) return -1;
    store->grants[store->grant_count++] = *grant;
    return 0;
}

int edge_authz_memory_add_point(edge_authz_memory_store_t *store, const char *scope,
                                uint8_t sensitivity) {
    if (!store || !scope) return -1;
    for (size_t i = 0; i < store->point_count; i++) {
        if (strcmp(store->points[i].scope, scope) == 0) {
            store->points[i].sensitivity = sensitivity;
            return 0;
        }
    }
    if (store->point_count >= EDGE_AUTHZ_MEMORY_MAX_POINTS) return -1;
    copy_text(store->points[store->point_count].scope, EDGE_AUTHZ_SCOPE_SIZE, scope);
    store->points[store->point_count].sensitivity = sensitivity;
    store->point_count++;
    return 0;
}

int edge_authz_memory_set_publisher(edge_authz_memory_store_t *store, const char *app_id,
                                    uint8_t trust_level, bool verified) {
    if (!store || !app_id) return -1;
    for (size_t i = 0; i < store->publisher_count; i++) {
        if (strcmp(store->publishers[i].app_id, app_id) == 0) {
            store->publishers[i].trust_level = trust_level;
            store->publishers[i].verified = verified;
            return 0;
        }
    }
    if (store->publisher_count >= EDGE_AUTHZ_MEMORY_MAX_PUBLISHERS) return -1;
    copy_text(store->publishers[store->publisher_count].app_id, EDGE_AUTHZ_APP_ID_SIZE, app_id);
    store->publishers[store->publisher_count].trust_level = trust_level;
    store->publishers[store->publisher_count].verified = verified;
    store->publisher_count++;
    return 0;
}

static int memory_get_grant(void *ctx, const char *app_id, const char *capability,
                            edge_app_grant_t *out) {
    edge_authz_memory_store_t *store = (edge_authz_memory_store_t *)ctx;
    if (!store || !app_id || !capability || !out) return -1;
    for (size_t i = 0; i < store->grant_count; i++) {
        if (strcmp(store->grants[i].app_id, app_id) == 0 &&
            strcmp(store->grants[i].capability, capability) == 0) {
            *out = store->grants[i];
            return 1;
        }
    }
    return 0;
}

static int memory_get_point_profile(void *ctx, const char *scope,
                                    edge_point_profile_t *out) {
    edge_authz_memory_store_t *store = (edge_authz_memory_store_t *)ctx;
    if (!store || !scope || !out) return -1;
    for (size_t i = 0; i < store->point_count; i++) {
        if (strcmp(store->points[i].scope, scope) == 0) {
            *out = store->points[i];
            return 1;
        }
    }
    return 0;
}

static int memory_get_publisher_trust(void *ctx, const char *app_id,
                                      edge_publisher_trust_t *out) {
    edge_authz_memory_store_t *store = (edge_authz_memory_store_t *)ctx;
    if (!store || !app_id || !out) return -1;
    for (size_t i = 0; i < store->publisher_count; i++) {
        if (strcmp(store->publishers[i].app_id, app_id) == 0) {
            *out = store->publishers[i];
            return 1;
        }
    }
    return 0;
}

edge_authz_repo_t edge_authz_memory_repo(edge_authz_memory_store_t *store) {
    edge_authz_repo_t repo = {
        .ctx = store,
        .get_grant = memory_get_grant,
        .get_point_profile = memory_get_point_profile,
        .get_publisher_trust = memory_get_publisher_trust,
    };
    return repo;
}
