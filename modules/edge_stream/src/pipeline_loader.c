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
extern pipeline_stage_t *stage_reflex_logic_create(stream_control_ops_t control_ops, uint32_t bus_id,
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

#define MAX_CUSTOM_STAGE_FACTORIES 32
static struct {
    char type_name[32];
    stage_factory_create_fn factory_fn;
} g_custom_stage_factories[MAX_CUSTOM_STAGE_FACTORIES];
static size_t g_custom_stage_factory_count = 0;

int stream_engine_register_stage_factory(const char *type_name, stage_factory_create_fn factory_fn) {
    if (!type_name || !factory_fn) return -1;
    for (size_t i = 0; i < g_custom_stage_factory_count; i++) {
        if (strcmp(g_custom_stage_factories[i].type_name, type_name) == 0) {
            g_custom_stage_factories[i].factory_fn = factory_fn;
            return 0;
        }
    }
    if (g_custom_stage_factory_count >= MAX_CUSTOM_STAGE_FACTORIES) return -2;
    strncpy(g_custom_stage_factories[g_custom_stage_factory_count].type_name, type_name, 31);
    g_custom_stage_factories[g_custom_stage_factory_count].factory_fn = factory_fn;
    g_custom_stage_factory_count++;
    return 0;
}

extern pipeline_stage_t *stage_energy_integrator_create(uint32_t state_tag_id);
extern pipeline_stage_t *stage_dsp_rms_create(uint32_t output_tag_id);

static pipeline_stage_t *build_stage(const char *type, cJSON *config, pipeline_loader_ctx_t *ctx) {
    if (!type) return NULL;

    // 1. 优先在动态注册的算子工厂表中查找
    for (size_t i = 0; i < g_custom_stage_factory_count; i++) {
        if (strcmp(g_custom_stage_factories[i].type_name, type) == 0) {
            return g_custom_stage_factories[i].factory_fn(type, config, ctx);
        }
    }

    // 2. 内建算子表
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
        return stage_reflex_logic_create(ctx->control_ops, bus_id, proto, target, threshold, gt,
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
            cJSON *type_item = cJSON_GetObjectItem(s_json, "type");
            const char *type = type_item ? type_item->valuestring : NULL;
            if (!type) continue;
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

// --- [Topology] DAG 拓扑无环校验 (DFS / Tarjan 算法) ---
static bool _dag_dfs_has_cycle(dag_graph_t *graph, uint32_t node_idx, bool *visited, bool *rec_stack) {
    visited[node_idx] = true;
    rec_stack[node_idx] = true;

    dag_node_t *node = &graph->nodes[node_idx];
    for (uint32_t i = 0; i < node->child_count; i++) {
        dag_node_t *child = node->children[i];
        if (!child) continue;
        uint32_t child_idx = (uint32_t)(child - graph->nodes);
        if (child_idx >= graph->node_count) continue;

        if (!visited[child_idx]) {
            if (_dag_dfs_has_cycle(graph, child_idx, visited, rec_stack)) return true;
        } else if (rec_stack[child_idx]) {
            return true; // 发现回边 (Cycle)
        }
    }

    rec_stack[node_idx] = false;
    return false;
}

int dag_validate_acyclic(dag_graph_t *graph) {
    if (!graph || graph->node_count == 0) return -1;

    bool visited[16] = {0};
    bool rec_stack[16] = {0};

    for (uint32_t i = 0; i < graph->node_count; i++) {
        if (!visited[i]) {
            if (_dag_dfs_has_cycle(graph, i, visited, rec_stack)) {
                graph->is_acyclic = false;
                return -1; // 存在环
            }
        }
    }
    graph->is_acyclic = true;
    return 0;
}

// --- 声明式 DAG 契约加载引擎 ---

int stream_engine_load_yaml_contract(stream_engine_t *engine, const char *yaml_content,
                                     pipeline_loader_ctx_t *ctx) {
    if (!engine || !yaml_content) return -1;

    cJSON *root = cJSON_Parse(yaml_content);
    if (!root) return -2;

    cJSON *pipelines = cJSON_GetObjectItem(root, "pipelines");
    if (!pipelines) {
        cJSON_Delete(root);
        return -3;
    }

    cJSON *p_json;
    cJSON_ArrayForEach(p_json, pipelines) {
        cJSON *id_item = cJSON_GetObjectItem(p_json, "id");
        cJSON *trig_item = cJSON_GetObjectItem(p_json, "trigger_tag");
        if (!id_item || !id_item->valuestring) continue;

        uint32_t trigger_tag = trig_item ? (uint32_t)trig_item->valuedouble : 0;
        pipeline_t *p = stream_engine_add_pipeline(engine, id_item->valuestring, trigger_tag);
        if (!p) continue;

        cJSON *nodes_json = cJSON_GetObjectItem(p_json, "nodes");
        if (nodes_json && cJSON_IsArray(nodes_json)) {
            // --- Declarative DAG Mode ---
            int node_count = cJSON_GetArraySize(nodes_json);
            if (node_count > 16) node_count = 16;

            typedef struct {
                char name[32];
                char type[32];
                cJSON *config;
                char input_names[4][32];
                int input_count;
                pipeline_stage_t *stage;
            } dag_node_meta_t;

            dag_node_meta_t meta[16];
            memset(meta, 0, sizeof(meta));

            dag_graph_t graph;
            memset(&graph, 0, sizeof(graph));
            graph.node_count = (uint32_t)node_count;

            int idx = 0;
            cJSON *n_json;
            cJSON_ArrayForEach(n_json, nodes_json) {
                if (idx >= 16) break;
                cJSON *name_item = cJSON_GetObjectItem(n_json, "name");
                cJSON *type_item = cJSON_GetObjectItem(n_json, "type");
                cJSON *cfg_item = cJSON_GetObjectItem(n_json, "config");
                if (!cfg_item) cfg_item = cJSON_GetObjectItem(n_json, "params");

                if (name_item && name_item->valuestring) {
                    strncpy(meta[idx].name, name_item->valuestring, 31);
                } else {
                    snprintf(meta[idx].name, 31, "node_%d", idx);
                }

                if (type_item && type_item->valuestring) {
                    strncpy(meta[idx].type, type_item->valuestring, 31);
                }
                meta[idx].config = cfg_item;

                cJSON *inputs_json = cJSON_GetObjectItem(n_json, "inputs");
                if (inputs_json && cJSON_IsArray(inputs_json)) {
                    cJSON *in_name;
                    cJSON_ArrayForEach(in_name, inputs_json) {
                        if (meta[idx].input_count < 4 && in_name->valuestring) {
                            strncpy(meta[idx].input_names[meta[idx].input_count], in_name->valuestring, 31);
                            meta[idx].input_count++;
                        }
                    }
                }

                // 实例化 stage
                meta[idx].stage = build_stage(meta[idx].type, meta[idx].config, ctx);
                if (meta[idx].stage) {
                    strncpy(meta[idx].stage->name, meta[idx].name, sizeof(meta[idx].stage->name) - 1);
                }
                graph.nodes[idx].stage = meta[idx].stage;
                idx++;
            }

            // 构建图拓扑边 (pred -> curr)
            for (int i = 0; i < node_count; i++) {
                for (int in = 0; in < meta[i].input_count; in++) {
                    const char *pred_name = meta[i].input_names[in];
                    // 查找前驱节点索引
                    for (int j = 0; j < node_count; j++) {
                        if (strcmp(meta[j].name, pred_name) == 0) {
                            // j 是前驱, i 是后继 (j -> i)
                            if (graph.nodes[j].child_count < 8) {
                                graph.nodes[j].children[graph.nodes[j].child_count++] = &graph.nodes[i];
                                graph.nodes[i].in_degree++;
                            }
                            break;
                        }
                    }
                }
            }

            // 拓扑无环性验证 (Kahn / DFS Cycle Check)
            int val_rc = dag_validate_acyclic(&graph);
            if (val_rc != 0) {
                // 释放已分配的 stages
                for (int i = 0; i < node_count; i++) {
                    if (meta[i].stage) {
                        if (meta[i].stage->cleanup_state && meta[i].stage->priv_state) {
                            meta[i].stage->cleanup_state(meta[i].stage->priv_state);
                        }
                        if (meta[i].stage->cleanup_cfg && meta[i].stage->priv_cfg) {
                            meta[i].stage->cleanup_cfg(meta[i].stage->priv_cfg);
                        }
                        free(meta[i].stage);
                    }
                }
                cJSON_Delete(root);
                return -22; // -EINVAL 环路错误
            }

            // 自动连线 next 分支指针
            for (int j = 0; j < node_count; j++) {
                if (graph.nodes[j].stage) {
                    for (uint32_t c = 0; c < graph.nodes[j].child_count && c < 4; c++) {
                        graph.nodes[j].stage->next[c] = graph.nodes[j].children[c]->stage;
                    }
                }
            }

            // 找到入度为 0 的首个节点作为 pipeline head
            pipeline_stage_t *head_stage = NULL;
            for (int i = 0; i < node_count; i++) {
                if (graph.nodes[i].in_degree == 0 && graph.nodes[i].stage) {
                    head_stage = graph.nodes[i].stage;
                    break;
                }
            }
            if (!head_stage && node_count > 0) {
                head_stage = graph.nodes[0].stage;
            }

            if (head_stage) {
                p->head = head_stage;
                p->tail = head_stage;
            }
        } else {
            // --- Linear Stages Fallback ---
            cJSON *stages_json = cJSON_GetObjectItem(p_json, "stages");
            cJSON *s_json;
            cJSON_ArrayForEach(s_json, stages_json) {
                cJSON *type_item = cJSON_GetObjectItem(s_json, "type");
                const char *type = type_item ? type_item->valuestring : NULL;
                if (!type) continue;
                cJSON *cfg_json = cJSON_GetObjectItem(s_json, "config");
                if (!cfg_json) cfg_json = cJSON_GetObjectItem(s_json, "params");
                pipeline_stage_t *s = build_stage(type, cfg_json, ctx);
                if (s) {
                    pipeline_add_stage(p, s->name, s->process, s->priv_state, s->cleanup_state,
                                       s->priv_cfg, s->cleanup_cfg);
                    free(s);
                }
            }
        }
    }

    cJSON_Delete(root);
    return 0;
}

int stream_engine_load_yaml_file(stream_engine_t *engine, const char *yaml_path,
                                 pipeline_loader_ctx_t *ctx) {
    if (!engine || !yaml_path) return -1;
    FILE *f = fopen(yaml_path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc((size_t)len + 1);
    if (!data) {
        fclose(f);
        return -2;
    }
    size_t read_bytes = fread(data, 1, (size_t)len, f);
    fclose(f);
    data[read_bytes] = '\0';

    int rc = stream_engine_load_yaml_contract(engine, data, ctx);
    free(data);
    return rc;
}

char *stream_engine_export_dot(stream_engine_t *engine) {
    if (!engine) return NULL;
    char *buf = malloc(8192);
    if (!buf) return NULL;
    size_t pos = 0;
    pos += (size_t)snprintf(buf + pos, 8192 - pos, "digraph StreamEngine {\n  rankdir=LR;\n  node [shape=box, style=rounded];\n");

    uv_mutex_lock(&engine->pipe_mutex);
    struct uv__queue *q;
    uv__queue_foreach(q, &engine->pipelines) {
        pipeline_t *p = uv__queue_data(q, pipeline_t, node);
        pos += (size_t)snprintf(buf + pos, 8192 - pos, "  \"pipe_%s\" [shape=ellipse, label=\"Pipeline: %s\\nTrigger: 0x%X\"];\n",
                                p->id, p->id, p->trigger_tag);
        if (p->head) {
            pos += (size_t)snprintf(buf + pos, 8192 - pos, "  \"pipe_%s\" -> \"%s\";\n", p->id, p->head->name);
            pipeline_stage_t *s = p->head;
            while (s && pos < 8000) {
                for (int b = 0; b < 4; b++) {
                    if (s->next[b]) {
                        pos += (size_t)snprintf(buf + pos, 8192 - pos, "  \"%s\" -> \"%s\" [label=\"next[%d]\"];\n",
                                                s->name, s->next[b]->name, b);
                    }
                }
                s = s->next[0];
            }
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);

    pos += (size_t)snprintf(buf + pos, 8192 - pos, "}\n");
    return buf;
}