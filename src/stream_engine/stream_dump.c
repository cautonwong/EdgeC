#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>

#include "stream_engine_priv.h"

char *stream_engine_dump_json(stream_engine_t *engine) {
    if (!engine) return NULL;

    cJSON *root = cJSON_CreateArray();
    uv_mutex_lock(&engine->pipe_mutex);

    for (int i = 0; i < engine->pipe_count; i++) {
        pipeline_t *p = engine->pipelines[i];
        if (!p) continue;

        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "id", p->id);
        cJSON_AddNumberToObject(item, "trigger_tag", p->trigger_tag);

        // 状态转字符串
        const char *state_str = "UNKNOWN";
        if (p->state == PIPE_IDLE)
            state_str = "IDLE";
        else if (p->state == PIPE_RUNNING)
            state_str = "RUNNING";
        else if (p->state == PIPE_SUSPENDED)
            state_str = "SUSPENDED";
        cJSON_AddStringToObject(item, "state", state_str);

        // Stages
        cJSON *stages = cJSON_AddArrayToObject(item, "stages");
        pipeline_stage_t *curr = p->head;
        while (curr) {
            cJSON *s = cJSON_CreateObject();
            cJSON_AddStringToObject(s, "type", curr->name);
            // Config 很难反序列化，暂时略过或存简要信息
            cJSON_AddItemToArray(stages, s);
            curr = curr->next;
        }

        cJSON_AddItemToArray(root, item);
    }

    uv_mutex_unlock(&engine->pipe_mutex);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}
