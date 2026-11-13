/**
 * @file iems_app_host_ports.c
 * @brief Host ABI ports backed by DataHub shadow, app point slots and authz.
 */

#include "iems_app_host_ports.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <uv.h>

#include "app_points_sqlite.h"
#include "edge_authz_sqlite.h"
#include "edge_log.h"
#include "edgeos/edge_authz.h"
#include "pipe_msg.h"

static int fail(char *reason, size_t cap, const char *code) {
    if (reason && cap) snprintf(reason, cap, "%s", code);
    return -1;
}

static int authz_check(iems_app_host_ctx_t *ctx, const char *capability, const char *scope,
                       char *reason, size_t cap) {
    edge_authz_repo_t repo = edge_authz_sqlite(ctx->db);
    edge_authz_engine_t engine;
    edge_authz_engine_init(&engine, &repo);
    edge_authz_engine_set_enforcement(&engine, true);
    edge_authz_result_t result =
        edge_authz_check(&engine, ctx->app_id, capability, scope, (uint64_t)time(NULL));
    if (result == EDGE_AUTHZ_OK) return 0;
    const char *code = edge_authz_result_code(result);
    return fail(reason, cap, code ? code : "internal");
}

static const edge_tag_meta_t *product_tag_by_id(const iems_app_host_ctx_t *ctx, uint32_t id) {
    for (uint32_t i = 0; i < ctx->product_tag_count; i++) {
        if (ctx->product_tags[i].id == id) return &ctx->product_tags[i];
    }
    return NULL;
}

static const edge_tag_meta_t *product_tag_by_name(const iems_app_host_ctx_t *ctx,
                                                  const char *name) {
    for (uint32_t i = 0; i < ctx->product_tag_count; i++) {
        if (ctx->product_tags[i].name && strcmp(ctx->product_tags[i].name, name) == 0) {
            return &ctx->product_tags[i];
        }
    }
    return NULL;
}

/* Shadow timestamps are nanoseconds; the app ABI uses milliseconds. */
static uint64_t shadow_ts_to_ms(uint64_t ts) {
    return ts > 100000000000000ULL ? ts / 1000000ULL : ts;
}

static uint8_t quality_from_shadow(uint8_t quality) {
    if (quality == 1) return APP_HOST_QUALITY_BAD;
    if (quality == 2) return APP_HOST_QUALITY_UNCERTAIN;
    return APP_HOST_QUALITY_GOOD;
}

static uint8_t quality_to_shadow(uint8_t quality) {
    if (quality == APP_HOST_QUALITY_BAD) return 1;
    if (quality == APP_HOST_QUALITY_UNCERTAIN) return 2;
    return 0;
}

static int host_resolve(void *ctx, const char *name, uint32_t *handle, uint8_t *access,
                        char *reason, size_t reason_size) {
    iems_app_host_ctx_t *self = (iems_app_host_ctx_t *)ctx;
    if (!self || !name) return fail(reason, reason_size, "invalid");

    if (strncmp(name, "app.", 4) == 0) {
        char owner[64] = "";
        int slot = 0;
        int found = app_points_find_full(self->db, name, owner, sizeof(owner), &slot);
        if (found < 0) return fail(reason, reason_size, "internal");
        if (found == 0) return fail(reason, reason_size, "not_found");
        *handle = self->app_base + (uint32_t)slot;
        *access = strcmp(owner, self->app_id) == 0 ? APP_HOST_ACCESS_PUBLISH
                                                   : APP_HOST_ACCESS_READ;
        return 0;
    }

    const edge_tag_meta_t *tag = product_tag_by_name(self, name);
    if (!tag) return fail(reason, reason_size, "not_found");
    *handle = tag->id;
    *access = APP_HOST_ACCESS_READ;
    return 0;
}

static int host_read(void *ctx, uint32_t handle, app_host_sample_t *out, char *reason,
                     size_t reason_size) {
    iems_app_host_ctx_t *self = (iems_app_host_ctx_t *)ctx;
    if (!self || !out || !self->hub) return fail(reason, reason_size, "internal");

    char scope[APP_SPEC_POINT_FULL_SIZE];
    if (handle >= self->app_base) {
        char owner[64] = "";
        char name[APP_SPEC_POINT_NAME_SIZE] = "";
        int slot = (int)(handle - self->app_base);
        int found = app_points_find_slot(self->db, slot, owner, sizeof(owner), name, sizeof(name));
        if (found < 0) return fail(reason, reason_size, "internal");
        if (found == 0) return fail(reason, reason_size, "not_found");
        if (strcmp(owner, self->app_id) == 0) {
            /* An app reads its own points only through a write handle. */
            return fail(reason, reason_size, "denied_scope");
        }
        snprintf(scope, sizeof(scope), "app.%s.%s", owner, name);
    } else {
        const edge_tag_meta_t *tag = product_tag_by_id(self, handle);
        if (!tag || !tag->name) return fail(reason, reason_size, "not_found");
        snprintf(scope, sizeof(scope), "%s", tag->name);
    }

    if (authz_check(self, "tags.read", scope, reason, reason_size) != 0) return -1;

    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    if (datahub_read_shadow(self->hub, handle, &msg) != 0 || msg.type != PIPEMSG_TYPE_SCALAR) {
        return fail(reason, reason_size, "stale");
    }
    memset(out, 0, sizeof(*out));
    out->kind = APP_HOST_VALUE_FLOAT;
    out->number = msg.payload.val.f;
    out->quality = quality_from_shadow(msg.quality);
    out->source_ts = shadow_ts_to_ms(msg.ts);
    return 0;
}

static int host_publish(void *ctx, uint32_t handle, const app_host_sample_t *sample, char *reason,
                        size_t reason_size) {
    iems_app_host_ctx_t *self = (iems_app_host_ctx_t *)ctx;
    if (!self || !sample || !self->hub) return fail(reason, reason_size, "internal");
    if (sample->kind == APP_HOST_VALUE_TEXT) {
        return fail(reason, reason_size, "invalid");
    }
    if (handle < self->app_base) return fail(reason, reason_size, "denied_scope");

    char owner[64] = "";
    char name[APP_SPEC_POINT_NAME_SIZE] = "";
    int slot = (int)(handle - self->app_base);
    int found = app_points_find_slot(self->db, slot, owner, sizeof(owner), name, sizeof(name));
    if (found < 0) return fail(reason, reason_size, "internal");
    if (found == 0) return fail(reason, reason_size, "not_found");
    if (strcmp(owner, self->app_id) != 0) return fail(reason, reason_size, "denied_scope");

    char scope[APP_SPEC_POINT_FULL_SIZE];
    snprintf(scope, sizeof(scope), "app.%s.%s", owner, name);
    if (authz_check(self, "points.publish", scope, reason, reason_size) != 0) return -1;

    pipe_msg_t msg;
    double value = sample->kind == APP_HOST_VALUE_INTEGER ? (double)sample->integer
                                                          : (sample->kind == APP_HOST_VALUE_BOOLEAN
                                                                 ? (sample->boolean ? 1.0 : 0.0)
                                                                 : sample->number);
    pipe_msg_init_scalar(&msg, handle, value, uv_hrtime());
    msg.quality = quality_to_shadow(sample->quality);
    if (datahub_write_shadow(self->hub, &msg) != 0) {
        return fail(reason, reason_size, "internal");
    }
    return 0;
}

static void host_log(void *ctx, int level, const char *message) {
    iems_app_host_ctx_t *self = (iems_app_host_ctx_t *)ctx;
    int log_level = level < 0 ? 0 : (level > 3 ? 3 : level);
    edge_log_out(NULL, LOG_CAT_CORE, log_level, "app=%s: %s", self ? self->app_id : "?",
                 message ? message : "");
}

static uint64_t host_now_ms(void *ctx) {
    (void)ctx;
    return (uint64_t)time(NULL) * 1000ULL;
}

void iems_app_host_ports_init(app_host_ports_t *ports, iems_app_host_ctx_t *ctx) {
    if (!ports || !ctx) return;
    memset(ports, 0, sizeof(*ports));
    ports->ctx = ctx;
    ports->resolve = host_resolve;
    ports->read = host_read;
    ports->publish = host_publish;
    ports->log = host_log;
    ports->now_ms = host_now_ms;
}
