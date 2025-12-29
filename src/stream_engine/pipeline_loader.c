#include <cJSON/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream_engine_priv.h"

extern pipeline_stage_t *stage_filter_create(uint32_t tag_id);
extern pipeline_stage_t *stage_accumulate_create(void);
extern pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table);

static pipeline_stage_t *build_stage(const char *type, cJSON *config, void *db_context) {
    if (strcmp(type, "filter") == 0) {
        cJSON *item = cJSON_GetObjectItem(config, "tag_id");
        uint32_t tag = (item) ? (uint32_t)item->valuedouble : 0;
        return stage_filter_create(tag);
    }
    if (strcmp(type, "accumulate") == 0) {
        return stage_accumulate_create();
    }
    if (strcmp(type, "sqlite_sink") == 0) {
        cJSON *item = cJSON_GetObjectItem(config, "table");
        const char *table = (item) ? item->valuestring : "default_table";
        return stage_sqlite_sink_create((sqlite_svc_t *)db_context, table);
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
    cJSON *pipeline_json;
    cJSON_ArrayForEach(pipeline_json, pipelines) {
        cJSON *id_item = cJSON_GetObjectItem(pipeline_json, "id");
        cJSON *trigger_item = cJSON_GetObjectItem(pipeline_json, "trigger_tag");

        if (!id_item || !trigger_item) continue;

        const char *id = id_item->valuestring;
        uint32_t trigger = (uint32_t)trigger_item->valuedouble;

        pipeline_t *p = stream_engine_add_pipeline(engine, id, trigger);
        if (!p) continue;

        cJSON *stages_json = cJSON_GetObjectItem(pipeline_json, "stages");
        cJSON *stage_json;
        cJSON_ArrayForEach(stage_json, stages_json) {
            cJSON *type_item = cJSON_GetObjectItem(stage_json, "type");
            if (!type_item) continue;

            const char *type = type_item->valuestring;
            cJSON *cfg = cJSON_GetObjectItem(stage_json, "config");
            pipeline_stage_t *s = build_stage(type, cfg, db_context);
            if (s) {
                pipeline_add_stage(p, s->name, s->process, s->priv_state, s->priv_cfg, s->cleanup);
                printf("[Loader]   + Stage: %s added to Pipeline: %s\n", s->name, id);
                free(s);
            }
        }
    }
    cJSON_Delete(root);
    return 0;
}
