#include <uv.h>
#include "thread_port_uv.h"

#include "datahub.h"
#include "edge_composition_root.h"
#include "edge_log.h"
#include "smartbox_ble_adapter.h"
#include "smartbox_driver_adapter.h"
#include "smartbox_hw_io_adapter.h"
#include "smartbox_north_adapter.h"
#include "smartbox_peripheral_adapter.h"
#include "stream_bus_datahub_adapter.h"
#include "wasm_app_scheduler.h"
#include "wasm_mgr.h"

int main(int argc, char **argv) {
    edge_thread_port_install_uv();
    (void)argc;
    (void)argv;
    LOG_INFO(LOG_CAT_CORE, "=== Starting Smart Box Controller ===");
    uv_loop_t *loop = uv_default_loop();

    uint32_t tag_count = 0;
    const edge_tag_meta_t *tags = smartbox_get_tag_metadata(&tag_count);

    edge_composition_config_t comp_cfg = {.product_name = "smart_box",
                                          .loop = loop,
                                          .shm_name = "smart_box",
                                          .ring_capacity = 4096,
                                          .datahub_capacity = 1024,
                                          .tags = tags,
                                          .tag_count = tag_count,
                                          .map_fn_v2 = smartbox_tag_map_fn,
                                          .memory_pool_buffer_size = 2048,
                                          .memory_pool_capacity = 32,
                                          .enable_dma_buf = false,
                                          .enable_anti_corruption = true,
                                          .backend_type = DATAHUB_BACKEND_HYBRID};
    static edge_composition_container_storage_t comp_storage;
    edge_composition_container_t *comp_root =
        edge_composition_root_create_in(&comp_storage, &comp_cfg);
    if (!comp_root) {
        LOG_ERROR(LOG_CAT_CORE, "Failed to instantiate Composition Root");
        return -1;
    }
    datahub_t *hub = edge_composition_root_get_datahub(comp_root);

    smartbox_north_attach(hub);
    smartbox_hw_io_start(loop, hub);

    // Task 3: WASM Micro-Runtime 初始化沙箱容器调度器 (限制堆 256KB、栈 64KB、运行耗时门禁为 10ms)
    stream_bus_port_t wasm_bus = stream_bus_datahub_port(hub);
    wasm_mgr_init(&wasm_bus);
    wasm_scheduler_t *sched = wasm_scheduler_create(loop, 10);  // 10ms 心跳 (100Hz)
    if (sched) {
        // 限制堆 256KB (262144字节)，栈 64KB (65536字节)，执行超时 10ms
        wasm_scheduler_load_app(sched, "rehab_companion", "/data/apps/rehab_companion.wasm",
                                262144, 65536, 10);
        wasm_scheduler_start(sched);
    }

    // Task 9: 注册 NFC、BLE 与 Modbus 驱动定时轮询
    smartbox_peripheral_start(loop, hub);
    smartbox_ble_start(loop, hub);

    // 10pin 凤凰端子 4路输入/4路输出 IO 模拟轮询与硬件标记初始化
    pipe_msg_t msg_mode;
    pipe_msg_init_scalar(&msg_mode, TAG_BOX_CURRENT_SCENE, 1.0,
                         uv_hrtime());  // 默认场景 1.0 (老人守护场景)
    datahub_write_shadow(hub, &msg_mode);

    pipe_msg_t msg_phx;
    pipe_msg_init_scalar(&msg_phx, TAG_BLE_SLOT_BINDING_ID, 0x0F,
                         uv_hrtime());  // 4路槽位硬件连接标记
    datahub_write_shadow(hub, &msg_phx);

    LOG_INFO(
        LOG_CAT_CORE,
        "Smart Box Loop with HW polling, Wasm Scheduler, Phoenix 10pin IO and drivers initialized");
    int rc = uv_run(loop, UV_RUN_DEFAULT);

    smartbox_north_detach();
    if (sched) {
        wasm_scheduler_destroy(sched);
    }
    wasm_mgr_cleanup();
    edge_composition_root_destroy(comp_root);

    return rc;
}
