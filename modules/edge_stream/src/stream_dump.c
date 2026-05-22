#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream_engine_priv.h"

char *stream_engine_dump_json(stream_engine_t *engine) {
    cJSON *root = cJSON_CreateObject();
    cJSON *pipelines = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "pipelines", pipelines);

    uv_mutex_lock(&engine->pipe_mutex);
    struct uv__queue *q;
    uv__queue_foreach(q, &engine->pipelines) {
        pipeline_t *p = uv__queue_data(q, pipeline_t, node);
        cJSON *p_json = cJSON_CreateObject();
        cJSON_AddStringToObject(p_json, "id", p->id);
        cJSON_AddNumberToObject(p_json, "trigger_tag", p->trigger_tag);

        cJSON *stages = cJSON_CreateArray();
        pipeline_stage_t *curr = p->head;
        while (curr) {
            cJSON *s_json = cJSON_CreateObject();
            cJSON_AddStringToObject(s_json, "name", curr->name);
            cJSON_AddNumberToObject(s_json, "total_events", (double)curr->total_events);
            cJSON_AddNumberToObject(s_json, "last_proc_us", (double)curr->last_proc_us);
            cJSON_AddItemToArray(stages, s_json);
            curr = curr->next;
        }
        cJSON_AddItemToObject(p_json, "stages", stages);
        cJSON_AddItemToArray(pipelines, p_json);
    }
    uv_mutex_unlock(&engine->pipe_mutex);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}