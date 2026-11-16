typedef struct {
    const char *name;
    pipeline_stage_t *(*factory)(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
} edge_stage_factory_entry_t;

pipeline_stage_t *stage_filter_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)def; (void)ctx;
    return NULL;
}

static const edge_stage_factory_entry_t g_standard_stages[] = {
    {"filter", stage_filter_create_v2},
    {"filter", stage_filter_create_v2},
    {"ghost", stage_ghost_create_v2},
    {"alpha", stage_alpha_create_v2},
    {NULL, NULL}
};
