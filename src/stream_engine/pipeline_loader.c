#include <cJSON/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream_engine_priv.h"

/**
 * @brief Logic to map JSON strings to stage factory functions.
 */
static pipeline_stage_t *create_stage_by_type(const char *type, cJSON *config, void *context) {
    if (strcmp(type, "filter") == 0) {
        uint32_t tag = (uint32_t)cJSON_GetObjectItem(config, "tag_id")->valuedouble;
        return stage_filter_create(tag);
    } else if (strcmp(type, "accumulate") == 0) {
        return stage_accumulate_create();
    } else if (strcmp(type, "sqlite_sink") == 0) {
        const char *table = cJSON_GetObjectItem(config, "table")->valuestring;
        return stage_sqlite_sink_create((sqlite_svc_t *)context, table);
    }
    return NULL;
}

int stream_engine_load_config(stream_engine_t *engine, const char *json_path, void *db_context) {
    FILE *f = fopen(json_path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc((size_t)len + 1);
    if (!data) {
        fclose(f);
        return -1;
    }
    fread(data, 1, (size_t)len, f);
    fclose(f);
    data[len] = '\0';

    cJSON *root = cJSON_Parse(data);
    free(data);
    if (!root) return -2;

    cJSON *pipelines = cJSON_GetObjectItem(root, "pipelines");
    cJSON *pipeline;
    cJSON_ArrayForEach(pipeline, pipelines) {
        const char *id = cJSON_GetObjectItem(pipeline, "id")->valuestring;
        uint32_t trigger = (uint32_t)cJSON_GetObjectItem(pipeline, "trigger_tag")->valuedouble;

        pipeline_t *p = stream_engine_add_pipeline(engine, id, trigger);
        if (!p) continue;

        cJSON *stages = cJSON_GetObjectItem(pipeline, "stages");
        cJSON *stage;
        cJSON_ArrayForEach(stage, stages) {
            const char *type = cJSON_GetObjectItem(stage, "type")->valuestring;
            cJSON *cfg = cJSON_GetObjectItem(stage, "config");

            pipeline_stage_t *s = create_stage_by_type(type, cfg, db_context);
            if (s) {
                pipeline_add_stage(p, s->name, s->process, s->priv_state, s->cleanup);
                // Note: Standard stages created via factories have their own cfg/state.
                // We should ensure the engine's pipeline_add_stage handles this.
                // Refactoring note: better if factory returns a struct, or we use a more unified
                // 'add' API. For now, we free the 's' shell but keep state/cfg.
                pipeline_stage_t *last = p->head;
                while (last->next) last = last->next;
                last->priv_cfg = s->priv_cfg;
                free(s);
            }
        }
    }

    cJSON_Delete(root);
    return 0;
}
