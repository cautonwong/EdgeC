#ifndef EDGE_APP_MANIFEST_H
#define EDGE_APP_MANIFEST_H

#include <stddef.h>
#include <stdint.h>

#define APP_SPEC_ID_SIZE 64
#define APP_SPEC_VERSION_SIZE 32
#define APP_SPEC_PUBLISHER_ID_SIZE 64
#define APP_SPEC_LANGUAGE_SIZE 16
#define APP_SPEC_PROFILE_SIZE 16
#define APP_SPEC_ENTRY_SIZE 256
#define APP_SPEC_ARCH_SIZE 16
#define APP_SPEC_LICENSE_SIZE 64
#define APP_SPEC_MAX_SCOPES 32
#define APP_SPEC_SCOPE_SIZE 128
#define APP_SPEC_REASON_SIZE 160
#define APP_SPEC_MAX_PREVIEWS 8
#define APP_SPEC_PREVIEW_SIZE 32
#define APP_SPEC_MAX_POINTS 256
#define APP_SPEC_POINT_NAME_SIZE 64
#define APP_SPEC_POINT_UNIT_SIZE 16
#define APP_SPEC_POINT_FULL_SIZE 96
#define APP_SPEC_POINT_STALE_MIN_MS 100u
#define APP_SPEC_POINT_STALE_MAX_MS 3600000u
#define APP_SPEC_POINT_STALE_DEFAULT_MS 5000u

/** Defaults for optional fields; keep in sync with the manifest contract. */
#define APP_SPEC_DEFAULT_ARCH "any"
#define APP_SPEC_DEFAULT_LICENSE "proprietary"
#define APP_SPEC_DEFAULT_PRIORITY 5u
#define APP_SPEC_DEFAULT_PIDS_MAX 32u

/** Host policy for manifest features that ship disabled by default. */
typedef struct {
    int allow_preview_apps; /**< 0 rejects any non-empty preview list */
} app_manifest_policy_t;

typedef struct {
    uint32_t heap_kb;
    uint32_t storage_kb;
    uint32_t cpu_ms_per_sec;
    uint32_t calls_per_sec;
    uint32_t publish_per_sec;
    uint32_t pids_max; /**< limits.pidsMax, 1..1024 */
} app_limits_t;

typedef struct {
    char read[APP_SPEC_MAX_SCOPES][APP_SPEC_SCOPE_SIZE];
    size_t read_count;
    char publish[APP_SPEC_MAX_SCOPES][APP_SPEC_SCOPE_SIZE];
    size_t publish_count;
    int kv;
    int log;
    char tags_reason[APP_SPEC_REASON_SIZE];
    char points_reason[APP_SPEC_REASON_SIZE];
    char kv_reason[APP_SPEC_REASON_SIZE];
} app_capabilities_t;

/** Contributed telemetry point, contributes.telemetry.points. */
typedef enum {
    APP_POINT_NUMBER = 0,
    APP_POINT_INTEGER,
    APP_POINT_BOOLEAN,
    APP_POINT_TEXT,
} app_point_value_type_t;

typedef enum {
    APP_POINT_LATEST = 0,
    APP_POINT_LATEST_HISTORY,
} app_point_retention_t;

typedef enum {
    APP_POINT_STABLE = 0,
    APP_POINT_EXPERIMENTAL,
} app_point_stability_t;

typedef enum {
    APP_POINT_MEASURED = 0,
    APP_POINT_DERIVED,
} app_point_source_t;

typedef struct {
    char name[APP_SPEC_POINT_NAME_SIZE]; /**< short name, app.<id>.<name> is the full name */
    char unit[APP_SPEC_POINT_UNIT_SIZE];
    uint8_t type;      /**< app_point_value_type_t */
    uint8_t retention; /**< app_point_retention_t */
    uint8_t stability; /**< app_point_stability_t */
    uint8_t source;    /**< app_point_source_t */
    uint16_t contract_version;
    uint32_t stale_after_ms;
} app_point_spec_t;

/**
 * Typed app.yaml manifest. Covers the validated identity and resource fields;
 * optional sections without a consumer yet (contributes, requires, settings)
 * are accepted by the validator but not modeled here.
 */
typedef struct {
    int abi;
    char id[APP_SPEC_ID_SIZE];
    char version[APP_SPEC_VERSION_SIZE];
    char publisher_id[APP_SPEC_PUBLISHER_ID_SIZE];
    char language[APP_SPEC_LANGUAGE_SIZE];
    char execution_profile[APP_SPEC_PROFILE_SIZE];
    char entry[APP_SPEC_ENTRY_SIZE];
    app_limits_t limits;
    app_capabilities_t capabilities;
    /** requires.points, exact point names; every entry must be covered by tags.read. */
    char requires_points[APP_SPEC_MAX_SCOPES][APP_SPEC_SCOPE_SIZE];
    size_t requires_points_count;
    char arch[APP_SPEC_ARCH_SIZE]; /**< any | aarch64 | x86_64 | armv7 */
    char license[APP_SPEC_LICENSE_SIZE];
    uint32_t priority; /**< resources.priority, 0..10; higher wins */
    char preview[APP_SPEC_MAX_PREVIEWS][APP_SPEC_PREVIEW_SIZE]; /**< known preview features */
    size_t preview_count;
    /** contributes.telemetry.points; at most APP_SPEC_MAX_POINTS per app. */
    app_point_spec_t points[APP_SPEC_MAX_POINTS];
    size_t points_count;
} app_spec_t;

/** Validate an app.yaml manifest before registry mutation. */
int edge_app_manifest_validate_json(const char *json, char *reason, size_t reason_size);

/**
 * Validate with an explicit host policy. A NULL policy applies the defaults,
 * which deny every preview feature.
 */
int edge_app_manifest_validate_json_with_policy(const char *json,
                                                const app_manifest_policy_t *policy, char *reason,
                                                size_t reason_size);

/**
 * Parse and validate a manifest into a typed spec. Pass out == NULL to
 * validate only. Optional fields are filled with APP_SPEC_DEFAULT_* values.
 * Returns 0 on success, -1 with a reason on failure.
 */
int edge_app_manifest_parse(const char *json, app_spec_t *out, char *reason, size_t reason_size);

/** Parse variant that applies an explicit host policy; NULL policy uses defaults. */
int edge_app_manifest_parse_with_policy(const char *json, const app_manifest_policy_t *policy,
                                        app_spec_t *out, char *reason, size_t reason_size);

#endif /* EDGE_APP_MANIFEST_H */
