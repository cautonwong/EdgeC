#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battery_strategy.h"
#include "edge_params.h"
#include "stream_engine_priv.h"

extern pipeline_stage_t *stage_filter_create(uint32_t tag_id);
extern pipeline_stage_t *stage_accumulate_create(void);
extern pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table);
extern pipeline_stage_t *stage_transform_create(double alpha, double beta);
extern pipeline_stage_t *stage_ema_create(double alpha);
extern pipeline_stage_t *stage_clamp_create(double min, double max);
extern pipeline_stage_t *stage_peak_shaving_create(double threshold_w, uint32_t ctrl_tag);
extern pipeline_stage_t *stage_filter_range_create(double min, double max);
extern pipeline_stage_t *stage_periodic_freeze_create(int kind);
extern pipeline_stage_t *stage_time_guard_create(void);
extern pipeline_stage_t *stage_battery_guard_create(battery_limit_t *limits);
extern pipeline_stage_t *stage_reflex_logic_create(struct bus_orchestrator_s *orch, uint32_t bus_id,
                                                   const char *proto, uint32_t target_tag,
                                                   double threshold, bool greater_than,
                                                   double action_val);

// 辅助：从 JSON 或全局参数获取 double 值
static double get_cfg_double(cJSON *json, const char *key, edge_params_t *params,
                             const char *param_key, double def_val) {
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item) return item->valuedouble;
    if (params && param_key) return edge_param_get_float(params, param_key, def_val);
    return def_val;
}

extern pipeline_stage_t *stage_energy_integrator_create(uint32_t state_tag_id);
extern pipeline_stage_t *stage_dsp_rms_create(uint32_t output_tag_id);

static pipeline_stage_t *build_stage(const char *type, cJSON *config, pipeline_loader_ctx_t *ctx) {
    if (strcmp(type, "energy_integrator") == 0) {
        cJSON *item = cJSON_GetObjectItem(config, "state_tag");
        return stage_energy_integrator_create(item ? (uint32_t)item->valuedouble : 0);
    }
    if (strcmp(type, "dsp_rms") == 0) {
        cJSON *item = cJSON_GetObjectItem(config, "state_tag");
        return stage_dsp_rms_create(item ? (uint32_t)item->valuedouble : 0);
    }
    if (strcmp(type, "filter") == 0) {
        cJSON *item = cJSON_GetObjectItem(config, "tag_id");
        return stage_filter_create(item ? (uint32_t)item->valuedouble : 0);
    }
    if (strcmp(type, "periodic_freeze") == 0) {
        cJSON *item = cJSON_GetObjectItem(config, "kind");
        return stage_periodic_freeze_create(item ? (int)item->valuedouble : 1);
    }
    if (strcmp(type, "time_guard") == 0) {
        return stage_time_guard_create();
    }
    if (strcmp(type, "battery_guard") == 0) {
        battery_limit_t *limits = calloc(1, sizeof(battery_limit_t));
        // 优先从 JSON 读取，缺省则从全局参数读取
        limits->max_soc =
            (float)get_cfg_double(config, "max_soc", ctx->params, "batt_max_soc", 95.0);
        limits->min_soc =
            (float)get_cfg_double(config, "min_soc", ctx->params, "batt_min_soc", 10.0);
        limits->max_charge_pwr =
            (float)get_cfg_double(config, "max_charge_w", ctx->params, "batt_max_charge", 5000.0);
        limits->max_discharge_pwr = (float)get_cfg_double(config, "max_discharge_w", ctx->params,
                                                          "batt_max_discharge", 5000.0);
        return stage_battery_guard_create(limits);
    }
    if (strcmp(type, "transform") == 0) {
        cJSON *alpha_obj = cJSON_GetObjectItem(config, "alpha");
        cJSON *beta_obj = cJSON_GetObjectItem(config, "beta");
        double alpha = alpha_obj ? alpha_obj->valuedouble : 1.0;
        double beta = beta_obj ? beta_obj->valuedouble : 0.0;
        return stage_transform_create(alpha, beta);
    }
    if (strcmp(type, "ema") == 0) {
        double alpha = cJSON_GetObjectItem(config, "alpha")
                           ? cJSON_GetObjectItem(config, "alpha")->valuedouble
                           : 0.5;
        return stage_ema_create(alpha);
    }
    if (strcmp(type, "clamp") == 0) {
        double min = cJSON_GetObjectItem(config, "min")
                         ? cJSON_GetObjectItem(config, "min")->valuedouble
                         : -1e9;
        double max = cJSON_GetObjectItem(config, "max")
                         ? cJSON_GetObjectItem(config, "max")->valuedouble
                         : 1e9;
        return stage_clamp_create(min, max);
    }
    if (strcmp(type, "peak_shave") == 0) {
        double threshold = cJSON_GetObjectItem(config, "threshold")
                               ? cJSON_GetObjectItem(config, "threshold")->valuedouble
                               : 5000.0;
        uint32_t ctrl_tag = cJSON_GetObjectItem(config, "ctrl_tag")
                                ? (uint32_t)cJSON_GetObjectItem(config, "ctrl_tag")->valuedouble
                                : 0;
        return stage_peak_shaving_create(threshold, ctrl_tag);
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
            bool is_async = cJSON_IsTrue(cJSON_GetObjectItem(config, "async"));
            return stage_js_rule_create(ctx->js_engine, item->valuestring, is_async);
        }
    }
    if (strcmp(type, "wasm_executor") == 0) {
        cJSON *wasm_path = cJSON_GetObjectItem(config, "module");
        cJSON *entry = cJSON_GetObjectItem(config, "entry");
        return stage_wasm_create(wasm_path ? wasm_path->valuestring : NULL,
                                 entry ? entry->valuestring : "on_event");
    }
    if (strcmp(type, "reflex") == 0) {
        cJSON *bus_id_obj = cJSON_GetObjectItem(config, "bus_id");
        cJSON *proto_obj = cJSON_GetObjectItem(config, "protocol");
        cJSON *target_obj = cJSON_GetObjectItem(config, "target_tag");
        cJSON *thresh_obj = cJSON_GetObjectItem(config, "threshold");
        cJSON *gt_obj = cJSON_GetObjectItem(config, "greater_than");
        
        uint32_t bus_id = bus_id_obj ? (uint32_t)bus_id_obj->valuedouble : 0;
        const char *proto = proto_obj ? proto_obj->valuestring : "";
        uint32_t target = target_obj ? (uint32_t)target_obj->valuedouble : 0;
        double threshold = thresh_obj ? thresh_obj->valuedouble : 0.0;
        bool gt = gt_obj ? cJSON_IsTrue(gt_obj) : false;
        
        double action_val = cJSON_GetObjectItem(config, "action_value")
                                ? cJSON_GetObjectItem(config, "action_value")->valuedouble
                                : 1.0;
        return stage_reflex_logic_create(ctx->orch, bus_id, proto, target, threshold, gt,
                                         action_val);
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