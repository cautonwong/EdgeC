#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battery_strategy.h"
#include "stream_engine_priv.h"

extern pipeline_stage_t *stage_filter_create(uint32_t tag_id);
extern pipeline_stage_t *stage_accumulate_create(void);
extern pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table);
extern pipeline_stage_t *stage_transform_create(double alpha, double beta);
extern pipeline_stage_t *stage_filter_range_create(double min, double max);
extern pipeline_stage_t *stage_js_rule_create(js_engine_t *engine, const char *script_path);
extern pipeline_stage_t *stage_periodic_freeze_create(int kind);
extern pipeline_stage_t *stage_battery_guard_create(battery_limit_t *limits);

static pipeline_stage_t *build_stage(const char *type, cJSON *config, pipeline_loader_ctx_t *ctx) {
    if (strcmp(type, "filter") == 0) {
        cJSON *item = cJSON_GetObjectItem(config, "tag_id");
        return stage_filter_create(item ? (uint32_t)item->valuedouble : 0);
    }
    if (strcmp(type, "periodic_freeze") == 0) {
        cJSON *item = cJSON_GetObjectItem(config, "kind");
        return stage_periodic_freeze_create(item ? (int)item->valuedouble : 1);
    }
    if (strcmp(type, "battery_guard") == 0) {
        // 实际开发中，limits 通常从 edge_params 加载
        battery_limit_t *limits = calloc(1, sizeof(battery_limit_t));
        limits->max_soc = (float)cJSON_GetObjectItem(config, "max_soc")->valuedouble;
        limits->min_soc = (float)cJSON_GetObjectItem(config, "min_soc")->valuedouble;
        limits->max_charge_pwr = (float)cJSON_GetObjectItem(config, "max_charge_w")->valuedouble;
        limits->max_discharge_pwr =
            (float)cJSON_GetObjectItem(config, "max_discharge_w")->valuedouble;
        return stage_battery_guard_create(limits);
    }
    if (strcmp(type, "transform") == 0) {
        double alpha = cJSON_GetObjectItem(config, "alpha")->valuedouble;
        double beta = cJSON_GetObjectItem(config, "beta")->valuedouble;
        return stage_transform_create(alpha, beta);
    }
    if (strcmp(type, "accumulate") == 0) {
        return stage_accumulate_create();
    }
    if (strcmp(type, "sqlite_sink") == 0) {
        cJSON *item = cJSON_GetObjectItem(config, "table");
        return stage_sqlite_sink_create(ctx->db, item ? item->valuestring : "history");
    }
    if (strcmp(type, "js_rule") == 0) {
        cJSON *item = cJSON_GetObjectItem(config, "script_path");
        if (item && ctx->js_engine) {
            return stage_js_rule_create(ctx->js_engine, item->valuestring);
        }
    }
    return NULL;
}

int stream_engine_load_config(stream_engine_t *engine, const char *json_path,
                              pipeline_loader_ctx_t *ctx) {
    FILE *f = fopen(json_path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc((size_t)len + 1);
    fread(data, 1, (size_t)len, f);
    fclose(f);
    data[len] = '\0';

    cJSON *root = cJSON_Parse(data);
    free(data);
    if (!root) return -2;

    cJSON *pipelines = cJSON_GetObjectItem(root, "pipelines");
    cJSON *p_json;
    cJSON_ArrayForEach(p_json, pipelines) {
        cJSON *id_item = cJSON_GetObjectItem(p_json, "id");
        cJSON *trig_item = cJSON_GetObjectItem(p_json, "trigger_tag");
        if (!id_item) continue;

        pipeline_t *p = stream_engine_add_pipeline(
            engine, id_item->valuestring, trig_item ? (uint32_t)trig_item->valuedouble : 0);

        cJSON *stages_json = cJSON_GetObjectItem(p_json, "stages");
        cJSON *s_json;
        cJSON_ArrayForEach(s_json, stages_json) {
            const char *type = cJSON_GetObjectItem(s_json, "type")->valuestring;
            cJSON *cfg_json = cJSON_GetObjectItem(s_json, "config");
            pipeline_stage_t *s = build_stage(type, cfg_json, ctx);
            if (s) {
                pipeline_add_stage(p, s->name, s->process, s->priv_state, s->cleanup_state,
                                   s->priv_cfg, s->cleanup_cfg);
                free(s);
            }
        }
    }
    cJSON_Delete(root);
    return 0;
}
