#include "contract_registry.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

static const edge_contract_t *g_active_contract = NULL;

int contract_register(const edge_contract_t *contract) {
    if (!contract) return -1;
    g_active_contract = contract;
    printf(" 📖 [Registry] Global Contract Registered: %s (v%s)\n", contract->name, contract->version);
    return 0;
}

const edge_contract_t *contract_get_active(void) {
    return g_active_contract;
}

__attribute__((weak)) const edge_tag_meta_t *contract_tag_get_meta(edge_tag_id_t id) {
    if (!g_active_contract) return NULL;
    
    // 此处可以优化为哈希查找，暂时使用线性查找
    for (uint32_t i = 0; i < g_active_contract->tag_count; i++) {
        if (g_active_contract->tags[i].id == id) {
            return &g_active_contract->tags[i];
        }
    }
    return NULL;
}

const api_route_t *contract_get_api_routes(uint32_t *count) {
    if (!g_active_contract) {
        *count = 0;
        return NULL;
    }
    *count = g_active_contract->api_route_count;
    return g_active_contract->api_routes;
}

// --- 💡 [ADR-0002] Contract Instance Management ---

static contract_instance_t *g_active_instance = NULL;
static contract_instance_t *g_instance_list = NULL;

contract_instance_t *contract_instance_load(const char *path) {
    if (!path) return NULL;

    FILE *f = fopen(path, "rb");
    if (!f) {
        printf(" [Contract-Blob] Failed to open: %s\n", path);
        return NULL;
    }

    // Read header: version(4) + name(64) + schema_id(64)
    contract_instance_t *inst = calloc(1, sizeof(contract_instance_t));
    if (!inst) { fclose(f); return NULL; }

    size_t n = fread(&inst->version, sizeof(uint32_t), 1, f);
    if (n != 1) { fclose(f); free(inst); return NULL; }

    n = fread(inst->name, 1, 64, f);
    n = fread(inst->schema_id, 1, 64, f);

    // Read tags
    n = fread(&inst->tag_count, sizeof(uint32_t), 1, f);
    if (inst->tag_count > 0) {
        inst->tag_table = calloc(inst->tag_count, sizeof(edge_tag_meta_t));
        n = fread(inst->tag_table, sizeof(edge_tag_meta_t), inst->tag_count, f);
    }

    // Read pipelines
    n = fread(&inst->pipeline_count, sizeof(uint32_t), 1, f);
    if (inst->pipeline_count > 0) {
        inst->pipeline_desc = calloc(inst->pipeline_count, sizeof(edge_pipeline_def_t));
        n = fread(inst->pipeline_desc, sizeof(edge_pipeline_def_t), inst->pipeline_count, f);
    }

    inst->loaded_at = (uint64_t)time(NULL);
    strncpy(inst->source_url, path, sizeof(inst->source_url) - 1);

    fclose(f);

    // Add to instance list
    inst->next = g_instance_list;
    g_instance_list = inst;

    printf(" [Contract-Blob] Loaded: %s v%u (tags=%u, pipelines=%u)\n",
           inst->name, inst->version, inst->tag_count, inst->pipeline_count);
    return inst;
}

int contract_instance_activate(contract_instance_t *inst) {
    if (!inst) return -1;
    inst->is_active = true;
    g_active_instance = inst;

    // 同步到全局契约注册中心（兼容现有 edge_contract_t 消费者）
    // Phase 2/3 将统一为 contract_instance_t 作为唯一契约句柄

    // [ADR-0002] 激活时自动触发 Pipeline 热重载
    printf(" [Contract-Blob] Activated: %s v%u\n", inst->name, inst->version);
    contract_pipeline_reload(inst);
    return 0;
}

void contract_instance_destroy(contract_instance_t *inst) {
    if (!inst) return;

    // Remove from list
    if (g_instance_list == inst) {
        g_instance_list = inst->next;
    } else {
        contract_instance_t *p = g_instance_list;
        while (p && p->next != inst) p = p->next;
        if (p) p->next = inst->next;
    }

    if (g_active_instance == inst) g_active_instance = NULL;

    if (inst->tag_table) free(inst->tag_table);
    if (inst->pipeline_desc) free(inst->pipeline_desc);
    free(inst);
}

contract_instance_t *contract_instance_get_active(void) {
    return g_active_instance;
}

// --- [ADR-0002] Pipeline Hot Reload ---

static contract_on_reload_fn g_reload_cb = NULL;
static void *g_reload_user_data = NULL;

void contract_set_reload_callback(contract_on_reload_fn fn, void *user_data) {
    g_reload_cb = fn;
    g_reload_user_data = user_data;
}

int contract_pipeline_prepare(contract_instance_t *inst) {
    (void)inst;
    // Pipeline 准备由 StreamEngine 层通过回调实现
    // 这里预留接口，实际构建逻辑在 pipeline_loader.c 中
    return 0;
}

int contract_pipeline_switch(contract_instance_t *inst) {
    if (!inst) return -1;
    // 原子切换到新 Pipeline
    if (g_reload_cb) {
        g_reload_cb(inst, g_reload_user_data);
    }
    return 0;
}

int contract_pipeline_reload(contract_instance_t *inst) {
    if (!inst) return -1;
    int rc = contract_pipeline_prepare(inst);
    if (rc != 0) return rc;
    return contract_pipeline_switch(inst);
}
