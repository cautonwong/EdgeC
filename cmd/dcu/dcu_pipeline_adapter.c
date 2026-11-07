#include <stdlib.h>

#include "dcu_event_monitor.h"
#include "dcu_pipeline_adapter.h"

// === [Consumer-Driven Interfaces] Wrappers ===
static int _event_archive_event(void *ctx, const char *addr, uint16_t event_code,
                                uint64_t start_time, uint64_t end_time, float snapshot_val) {
    return dcu_storage_write_event((dcu_storage_t *)ctx, addr, event_code, start_time, end_time,
                                   snapshot_val);
}

static int _event_archive_freeze(void *ctx, const char *addr, uint32_t freeze_oad,
                                 uint64_t freeze_time, uint32_t src_oad, double value) {
    return dcu_storage_write_freeze((dcu_storage_t *)ctx, addr, freeze_oad, freeze_time, src_oad,
                                    value);
}

void dcu_pipelines_attach(stream_engine_t *se, datahub_t *hub, dcu_inventory_t *inv,
                          dcu_node_t *node, dcu_storage_t *storage) {
    if (!se) return;

    event_sink_ops_t event_sink = {.ctx = storage,
                                   .archive_event = _event_archive_event,
                                   .archive_freeze = _event_archive_freeze};

    pipeline_t *evt_pipe = stream_engine_add_pipeline(se, "EVENT_PIPELINE", 0);
    pipeline_stage_t *m_stage = stage_event_monitor_create(inv, dcu_node_get_event_3000(node),
                                                           dcu_node_get_event_3005(node));
    pipeline_add_stage(evt_pipe, m_stage->name, m_stage->process, m_stage->priv_state,
                       m_stage->cleanup_state, NULL, NULL);
    free(m_stage);
    pipeline_stage_t *l_stage = stage_event_logger_create(
        inv, dcu_node_get_event_3000(node), dcu_node_get_event_3005(node), hub, event_sink);
    pipeline_add_stage(evt_pipe, l_stage->name, l_stage->process, l_stage->priv_state,
                       l_stage->cleanup_state, NULL, NULL);
    free(l_stage);

    pipeline_t *frz_pipe = stream_engine_add_pipeline(se, "FREEZE_PIPELINE", 0);
    pipeline_stage_t *f_stage = stage_periodic_freeze_create_ex(inv, hub, event_sink);
    pipeline_add_stage(frz_pipe, f_stage->name, f_stage->process, f_stage->priv_state,
                       f_stage->cleanup_state, NULL, NULL);
    free(f_stage);
}
