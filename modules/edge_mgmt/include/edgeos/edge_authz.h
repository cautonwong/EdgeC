/**
 * @file edge_authz.h
 * @brief Grant model and runtime authorization checks for the app platform.
 *
 * A grant is bound to (app_id, capability, scope_json). The engine reads
 * through an injected repository so T2 can back it with the app registry
 * without this file depending on SQLite.
 *
 * Sensitivity: 0 public, 1 normal, 2 sensitive, 3 critical. A grant with
 * sensitivity_max >= 2 requires a non-empty purpose. Scope matching is exact:
 * no wildcards are accepted or expanded.
 */
#ifndef EDGEOS_EDGE_AUTHZ_H
#define EDGEOS_EDGE_AUTHZ_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_AUTHZ_APP_ID_SIZE 64
#define EDGE_AUTHZ_CAPABILITY_SIZE 32
#define EDGE_AUTHZ_SCOPE_SIZE 128
#define EDGE_AUTHZ_SCOPE_JSON_SIZE 512
#define EDGE_AUTHZ_PURPOSE_SIZE 160
#define EDGE_AUTHZ_ACTOR_SIZE 64

/** Sensitivity at or above this value needs a purpose and trusted publisher. */
#define EDGE_AUTHZ_SENSITIVITY_SENSITIVE 2u
/** Minimum publisher trust_level for sensitive points. */
#define EDGE_AUTHZ_TRUST_SENSITIVE_MIN 2u

typedef enum {
    EDGE_GRANT_PENDING = 0,
    EDGE_GRANT_GRANTED = 1,
    EDGE_GRANT_DENIED = 2,
    EDGE_GRANT_SUSPENDED = 3,
} edge_grant_state_t;

typedef struct {
    char app_id[EDGE_AUTHZ_APP_ID_SIZE];
    char capability[EDGE_AUTHZ_CAPABILITY_SIZE];
    /** JSON array of exact scope strings, e.g. ["U_A","U_B"]. */
    char scope_json[EDGE_AUTHZ_SCOPE_JSON_SIZE];
    edge_grant_state_t state;
    char purpose[EDGE_AUTHZ_PURPOSE_SIZE];
    uint64_t expires_at;    /**< epoch seconds; 0 = never expires */
    uint8_t sensitivity_max; /**< highest point sensitivity this grant allows */
    char granted_by[EDGE_AUTHZ_ACTOR_SIZE];
    uint64_t granted_at;    /**< epoch seconds */
    uint64_t revoked_at;    /**< epoch seconds; 0 = not revoked */
} edge_app_grant_t;

typedef struct {
    char app_id[EDGE_AUTHZ_APP_ID_SIZE];
    uint8_t trust_level;
    bool verified;
} edge_publisher_trust_t;

typedef struct {
    char scope[EDGE_AUTHZ_SCOPE_SIZE];
    uint8_t sensitivity;
} edge_point_profile_t;

typedef enum {
    EDGE_AUTHZ_OK = 0,
    EDGE_AUTHZ_DENIED_NOT_GRANTED,
    EDGE_AUTHZ_DENIED_EXPIRED,
    EDGE_AUTHZ_DENIED_POLICY,
    EDGE_AUTHZ_DENIED_SCOPE,
    EDGE_AUTHZ_DENIED_QUOTA,
    EDGE_AUTHZ_DENIED_REVOKED,
    EDGE_AUTHZ_DENIED_PACKAGE_MISMATCH,
    EDGE_AUTHZ_INVALID_MANIFEST,
    EDGE_AUTHZ_STORE_ERROR,
} edge_authz_result_t;

/** Repository port. A getter returns 1 when found, 0 when absent, -1 on error. */
typedef struct {
    void *ctx;
    int (*get_grant)(void *ctx, const char *app_id, const char *capability,
                     edge_app_grant_t *out);
    int (*get_point_profile)(void *ctx, const char *scope, edge_point_profile_t *out);
    /** Resolve app_id to its publisher's trust record. */
    int (*get_publisher_trust)(void *ctx, const char *app_id, edge_publisher_trust_t *out);
} edge_authz_repo_t;

typedef struct {
    const edge_authz_repo_t *repo;
    bool enforce;
} edge_authz_engine_t;

/** Wire protocol code for a result, e.g. "denied_expired". */
const char *edge_authz_result_code(edge_authz_result_t result);

/** Check grant fields accepted by appctl grant. Sensitivity >= 2 needs a purpose. */
edge_authz_result_t edge_authz_validate_grant(const edge_app_grant_t *grant);

/** Evaluate one Host call. now is epoch seconds. */
edge_authz_result_t edge_authz_check(const edge_authz_engine_t *engine, const char *app_id,
                                     const char *capability, const char *scope, uint64_t now);

/** Compare the installed package digest with the running one; fail closed. */
edge_authz_result_t edge_authz_check_package(const char *installed_digest,
                                             const char *runtime_digest);

void edge_authz_engine_init(edge_authz_engine_t *engine, const edge_authz_repo_t *repo);

/** Kill switch: enforcement is off after init and returns OK for every check. */
void edge_authz_engine_set_enforcement(edge_authz_engine_t *engine, bool enabled);

/** Upgrade inheritance result, see docs/design/app-platform/02-authorization.md. */
typedef enum {
    EDGE_AUTHZ_INHERIT_UNCHANGED = 0, /**< same scope set, grant kept */
    EDGE_AUTHZ_INHERIT_NARROWED = 1,  /**< requested scopes are a subset, grant kept */
    EDGE_AUTHZ_INHERIT_AWAITING = 2,  /**< new or wider scopes, grant goes pending */
    EDGE_AUTHZ_INHERIT_REVOKED = 3,   /**< revoked grants never revive on upgrade */
    EDGE_AUTHZ_INHERIT_INVALID = 4,   /**< unparsable requested scope list */
    EDGE_AUTHZ_INHERIT_STORE_ERROR = 5, /**< store read or write failed */
} edge_authz_inherit_t;

/**
 * Apply an upgrade's requested scope list to the existing grant. Narrowing
 * shrinks the granted scope list; any new scope sends the capability to
 * EDGE_GRANT_PENDING so it stays unavailable until approved.
 */
edge_authz_inherit_t edge_authz_inherit_grant(const edge_app_grant_t *existing,
                                              const char *requested_scope_json,
                                              edge_app_grant_t *out);

/** Read/write grant store used by the admin path and by upgrades. */
typedef struct {
    void *ctx;
    int (*get_grant)(void *ctx, const char *app_id, const char *capability,
                     edge_app_grant_t *out);
    int (*save_grant)(void *ctx, const edge_app_grant_t *grant);
    int (*delete_grant)(void *ctx, const char *app_id, const char *capability);
    int (*list_grants)(void *ctx, const char *app_id, edge_app_grant_t *out, size_t max,
                       size_t *count);
    /** Record the signer trust of an installed app (8.16); 0 on success. */
    int (*set_publisher_trust)(void *ctx, const char *app_id, uint8_t trust_level, bool verified);
} edge_authz_store_t;

/**
 * Validate and persist an explicit admin approval. Stamps state GRANTED,
 * granted_at, clears revoked_at, then saves. granted_by must be set by the
 * caller.
 */
edge_authz_result_t edge_authz_approve_grant(const edge_authz_store_t *store,
                                             edge_app_grant_t *grant, uint64_t now);

/** Revoke a capability: load, set revoked_at, save. */
edge_authz_result_t edge_authz_revoke_grant(const edge_authz_store_t *store, const char *app_id,
                                            const char *capability, uint64_t now);

/**
 * Apply an app upgrade for one capability: load the existing grant and inherit
 * narrowed scopes, send new or wider scopes to pending, keep revoked grants
 * revoked, or create a pending grant when none exists. Saves the result.
 */
edge_authz_inherit_t edge_authz_inherit_upgrade(const edge_authz_store_t *store,
                                                const char *app_id, const char *capability,
                                                const char *requested_scope_json, uint64_t now);

/** In-memory repository for tests and for T2 before the SQLite port lands. */
#define EDGE_AUTHZ_MEMORY_MAX_GRANTS 32
#define EDGE_AUTHZ_MEMORY_MAX_POINTS 32
#define EDGE_AUTHZ_MEMORY_MAX_PUBLISHERS 16

typedef struct {
    edge_app_grant_t grants[EDGE_AUTHZ_MEMORY_MAX_GRANTS];
    size_t grant_count;
    edge_point_profile_t points[EDGE_AUTHZ_MEMORY_MAX_POINTS];
    size_t point_count;
    edge_publisher_trust_t publishers[EDGE_AUTHZ_MEMORY_MAX_PUBLISHERS];
    size_t publisher_count;
} edge_authz_memory_store_t;

void edge_authz_memory_init(edge_authz_memory_store_t *store);
/** Add or replace the grant for (app_id, capability). Returns 0, or -1 when full. */
int edge_authz_memory_add_grant(edge_authz_memory_store_t *store, const edge_app_grant_t *grant);
/** Add or replace a point profile. Returns 0, or -1 when full. */
int edge_authz_memory_add_point(edge_authz_memory_store_t *store, const char *scope,
                               uint8_t sensitivity);
/** Add or replace an app's publisher trust. Returns 0, or -1 when full. */
int edge_authz_memory_set_publisher(edge_authz_memory_store_t *store, const char *app_id,
                                    uint8_t trust_level, bool verified);
edge_authz_repo_t edge_authz_memory_repo(edge_authz_memory_store_t *store);

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_EDGE_AUTHZ_H */
