#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <uv.h>

#include "contract_registry.h"
#include "contracts.h"
#include "edge_api.h"
#include "edge_composition_root.h"
#include "edge_license.h"
#include "edge_service.h"
#include "modbus_codec.h"
#include "mqtt_producer.h"
#include "northbound_mgr.h"
#include "queue.h"
#include "rest_server.h"
#include "sched_time.h"
#include "sqlite_service.h"
#include "stream_engine.h"
#include "wasm_mgr.h"

#define MAX_VBUS 32

typedef enum {
    BUS_STATE_DISCONNECTED = 0,
    BUS_STATE_CONNECTING,
    BUS_STATE_CONNECTED,
    BUS_STATE_LISTENING
} bus_conn_state_t;

typedef struct {
    service_t* svc;
    datahub_t* hub;
    arbiter_t* arb;
    bus_orchestrator_t* orch;
    driver_manager_t* drv_mgr;
    stream_engine_t* stream_eng;
    northbound_mgr_t* north;
    sqlite_svc_t* db_service;
    char* db_path;
    edge_composition_container_t* comp_root;
} iems_app_t;

static iems_app_t g_app = {0};
static struct uv__queue g_poll_task_head;
static bool g_assets_loaded = false;
static bus_conn_state_t g_bus_state[MAX_VBUS] = {BUS_STATE_DISCONNECTED};

static uv_timer_t* g_timer_load_tasks = NULL;
static uv_timer_t* g_timer_vbus = NULL;
static uv_timer_t* g_timer_internal = NULL;

volatile bool g_adc_running = false;
static uv_thread_t g_adc_tid;

typedef struct {
    int bus_id;
    uint32_t slave_id;
    uint32_t tag_id;
    int dense_idx;  // 💡 [Nexus Physical-Path] 物理坑位索引
    char protocol[16];
    uint32_t interval_ms;
    uv_periodic_t scheduler;
    struct uv__queue node;
} iems_poll_task_t;

static void on_poll_complete(int status, const pipe_msg_t* result, void* user_data) {
    iems_poll_task_t* task = (iems_poll_task_t*)user_data;
    if (status == 0 && result && task && g_app.hub) {
        // 💡 [Nexus Physical-Path] 强行绕过查找，直接物理入库并写入 shm_ring 触发计算引擎！
        pipe_msg_t local_msg = *result;
        datahub_write_ring(g_app.hub, &local_msg);
        datahub_write_shadow_idx(g_app.hub, task->dense_idx, &local_msg);
    }
}

// [Consumer-Driven Adapter] 包装底层总线控制下发指令，传递给计算算子
static int _iems_stream_submit_control(void* ctx, int bus_id, uint32_t tag_id, double value,
                                       const char* proto) {
    (void)ctx;
    // 1. 立即更新 DataHub Shadow，保证状态立即可见（测试通过 REST API 可读到）
    if (g_app.hub) {
        pipe_msg_t ctrl_msg = {0};
        pipe_msg_init_scalar(&ctrl_msg, tag_id, value, 0);
        datahub_publish(g_app.hub, &ctrl_msg);
    }
    // 2. 异步走 Modbus 协议写入物理设备（南向默认 slave_id 为 1）
    return edge_api_write_tag(bus_id, proto, 1, tag_id, value, NULL, NULL);
}

static void on_ui_connect(uv_stream_t* server, int status) {
    if (status < 0) return;
    uv_pipe_t* client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        int cfd;
        uv_fileno((const uv_handle_t*)client, &cfd);
        edge_fd_send(cfd, datahub_get_ring_fd(g_app.hub));
        edge_fd_send(cfd, g_app.hub->shadow_fd);
    }
    uv_close((uv_handle_t*)client, (uv_close_cb)free);
}

static void on_poll_task_trigger(uv_periodic_t* handle) {
    iems_poll_task_t* task = (iems_poll_task_t*)handle->data;
    if (!task) return;
    if (!g_assets_loaded) return;

    const edge_contract_t* contract = contract_get_active();
    if (!contract) return;

    bool is_southbound = false;
    for (uint32_t i = 0; i < contract->connectivity_count; i++) {
        if (contract->connectivity[i].id == task->bus_id &&
            strcmp(contract->connectivity[i].category, "SOUTHBOUND") == 0) {
            is_southbound = true;
            break;
        }
    }

    if (is_southbound) {
        // 💡 [Nexus Fix] 废除不安全的取模，直接用物理 ID 索引
        if (task->bus_id < MAX_VBUS && g_bus_state[task->bus_id] == BUS_STATE_CONNECTED) {
            edge_api_read_tag(task->bus_id, task->protocol, task->slave_id, task->tag_id,
                              on_poll_complete, task);
        }
    }
}

#include "iems_task_loader.h"

static void _on_iems_task_loaded(const iems_loaded_task_t* loaded, void* user_data) {
    (void)user_data;
    if (!loaded) return;

    struct uv__queue* q;
    uv__queue_foreach(q, &g_poll_task_head) {
        iems_poll_task_t* existing = uv__queue_data(q, iems_poll_task_t, node);
        if (existing->bus_id == loaded->bus_id && existing->slave_id == loaded->slave_id &&
            existing->tag_id == loaded->tag_id)
            return;
    }

    iems_poll_task_t* task = calloc(1, sizeof(iems_poll_task_t));
    task->bus_id = loaded->bus_id;
    task->slave_id = loaded->slave_id;
    task->tag_id = loaded->tag_id;
    task->dense_idx = edge_tag_get_compact_index(loaded->tag_id);
    strncpy(task->protocol, loaded->protocol, 15);
    task->interval_ms = 1000;

    const edge_tag_meta_t* meta = contract_tag_get_meta(loaded->tag_id);
    if (meta && meta->poll_interval_ms > 0) {
        task->interval_ms = meta->poll_interval_ms;
    }

    printf("📝 [POLL REGISTER] Tag: %u, Bus: %d, Protocol: %s, Interval: %d\n", task->tag_id,
           task->bus_id, task->protocol, task->interval_ms);

    uv__queue_insert_tail(&g_poll_task_head, &task->node);
    uv_periodic_init(uv_default_loop(), &task->scheduler);
    task->scheduler.data = task;
    uv_periodic_set(&task->scheduler, 0, (double)task->interval_ms, NULL);
    uv_periodic_start(&task->scheduler, on_poll_task_trigger);
}

static void iems_load_tasks_timer_cb(uv_timer_t* handle) {
    if (!g_app.db_service) return;

    int loaded = iems_poll_tasks_load(g_app.db_service, _on_iems_task_loaded, NULL);
    if (loaded > 0) {
        g_assets_loaded = true;
        uv_timer_stop(handle);
        uv_close((uv_handle_t*)handle, (uv_close_cb)free);
        g_timer_load_tasks = NULL;
    }
}

typedef struct {
    int bus_id;
    int bus_idx;
} north_accept_ctx_t;

static void _on_north_connect(uv_stream_t* server, int status) {
    if (status < 0) return;
    north_accept_ctx_t* ctx = server->data;
    uv_pipe_t* client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        int fd;
        uv_fileno((const uv_handle_t*)client, &fd);
        int new_fd = dup(fd);
        if (new_fd >= 0) {
            int flags = fcntl(new_fd, F_GETFL, 0);
            fcntl(new_fd, F_SETFL, flags | O_NONBLOCK);
            io_handle_t* h = io_executor_create_from_fd(server->loop, new_fd, g_app.hub->io_pool);
            if (h) {
                bus_orchestrator_attach_io(g_app.orch, ctx->bus_id, h);
                if (h->ops && h->ops->enable_read) h->ops->enable_read(h, true);
            } else
                close(new_fd);
        }
    }
    uv_close((uv_handle_t*)client, (uv_close_cb)free);
}

static void _try_connect_buses(uv_timer_t* handle) {
    const edge_contract_t* contract = contract_get_active();
    if (!contract) return;

    for (uint32_t i = 0; i < contract->connectivity_count; i++) {
        const edge_connectivity_t* c = &contract->connectivity[i];
        // 💡 [Nexus Fix] 只有 0-9 号总线支持这种动态 UDS 连接方式
        if (c->id >= MAX_VBUS) continue;

        int bus_idx = (int)c->id;
        if (g_bus_state[bus_idx] != BUS_STATE_DISCONNECTED) continue;

        char env_key[32];
        snprintf(env_key, sizeof(env_key), "OMNIS_BUS_%d_DEV", bus_idx);
        const char* env_path = getenv(env_key);
        const char* path = env_path ? env_path : edge_resolve_bus_path(c->id);

        if (!path) continue;

        // 💡 [Omnis Final Fix] 暴力打通虚拟总线链路
        const char* search_paths[3];
        search_paths[0] = env_path;
        search_paths[1] = path;
        char fallback_path[64];
        snprintf(fallback_path, sizeof(fallback_path), "/tmp/vbus%d.sock", bus_idx);
        search_paths[2] = fallback_path;

        for (int p_idx = 0; p_idx < 3; p_idx++) {
            const char* cur_path = search_paths[p_idx];
            if (!cur_path || cur_path[0] == '\0') continue;

            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd < 0) continue;

            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, cur_path, sizeof(addr.sun_path) - 1);

            if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                io_handle_t* h = io_executor_create_from_fd(handle->loop, fd, g_app.hub->io_pool);
                if (h) {
                    bus_orchestrator_attach_io(g_app.orch, c->id, h);
                    if (h->ops && h->ops->enable_read) h->ops->enable_read(h, true);
                    g_bus_state[bus_idx] = BUS_STATE_CONNECTED;
                    printf("\033[32m[PHYSICS-SUCCESS] Bus %d CONNECTED via PATH: %s\033[0m\n",
                           bus_idx, cur_path);
                    break;
                } else
                    close(fd);
            } else {
                close(fd);
            }
        }
    }
}

extern void adc_driver_handle_connection(int sock_fd, datahub_t* hub);
typedef struct {
    int fd;
    datahub_t* hub;
} adc_thread_ctx_t;
static void _adc_thread_entry(void* arg) {
    adc_thread_ctx_t* ctx = (adc_thread_ctx_t*)arg;
    adc_driver_handle_connection(ctx->fd, ctx->hub);
    g_adc_running = false;
    free(ctx);
}
static void _try_connect_internal_buses(uv_timer_t* handle) {
    const edge_contract_t* contract = contract_get_active();
    if (!contract) return;

    for (uint32_t i = 0; i < contract->connectivity_count; i++) {
        const edge_connectivity_t* c = &contract->connectivity[i];
        if (strcmp(c->category, "INTERNAL") != 0 || g_adc_running) continue;
        const char* path = edge_resolve_bus_path(c->id);
        if (path && access(path, F_OK) == 0) {
            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
            if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                adc_thread_ctx_t* ctx = malloc(sizeof(adc_thread_ctx_t));
                ctx->fd = fd;
                ctx->hub = g_app.hub;
                g_adc_running = true;
                uv_thread_create(&g_adc_tid, _adc_thread_entry, ctx);
            } else
                close(fd);
        }
    }
}

static void _monitor_shadow_tick(uv_timer_t* handle) {
    (void)handle;
    if (g_app.hub && g_app.hub->shadow_table) {
        pipe_msg_t msg;
        if (datahub_read_shadow(g_app.hub, 1001, &msg) == 0) {
            printf("\033[36m[HEARTBEAT] Shadow Tag 1001 = %.2f\033[0m\n", msg.payload.val.f);
        }
    }
}

static void _on_task_close(uv_handle_t* handle) {
    uv_periodic_t* periodic = (uv_periodic_t*)handle->data;
    iems_poll_task_t* task = (iems_poll_task_t*)periodic->data;
    free(task);
}

static void iems_shutdown(uv_loop_t* loop) {
    if (g_timer_load_tasks) {
        uv_timer_stop(g_timer_load_tasks);
        uv_close((uv_handle_t*)g_timer_load_tasks, (uv_close_cb)free);
    }
    if (g_timer_vbus) {
        uv_timer_stop(g_timer_vbus);
        uv_close((uv_handle_t*)g_timer_vbus, NULL);
    }
    if (g_timer_internal) {
        uv_timer_stop(g_timer_internal);
        uv_close((uv_handle_t*)g_timer_internal, NULL);
    }
    while (!uv__queue_empty(&g_poll_task_head)) {
        struct uv__queue* q = uv__queue_head(&g_poll_task_head);
        iems_poll_task_t* task = uv__queue_data(q, iems_poll_task_t, node);
        uv__queue_remove(q);
        uv_periodic_stop(&task->scheduler);
        uv_close((uv_handle_t*)&task->scheduler.timer, _on_task_close);
    }
    uv_run(loop, UV_RUN_NOWAIT);
    if (g_adc_running) {
        g_adc_running = false;
        uv_thread_join(&g_adc_tid);
    }
    if (g_app.stream_eng) stream_engine_destroy(g_app.stream_eng);
    wasm_mgr_cleanup();
    if (g_app.svc) service_delete(g_app.svc);
    if (g_app.db_service) sqlite_svc_destroy(g_app.db_service);
    if (g_app.orch) bus_orchestrator_destroy(g_app.orch);
    if (g_app.arb) arbiter_destroy(g_app.arb);
    if (g_app.drv_mgr) driver_manager_destroy(g_app.drv_mgr);
    if (g_app.comp_root) {
        edge_composition_root_destroy(g_app.comp_root);
        g_app.hub = NULL;
    } else if (g_app.hub) {
        datahub_destroy(g_app.hub);
        free(g_app.hub);
    }
    if (g_app.db_path) free(g_app.db_path);
    curl_global_cleanup();
}

static void on_signal_exit(uv_signal_t* handle, int signum) {
    uv_stop(handle->loop);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // --- SDK Tiering & License Engine ---
    // In production, the key would be loaded from a secure element or environment
    const char* lic_key = getenv("EDGEC_LICENSE_KEY");
    if (license_engine_init(lic_key ? lic_key : "ENT-EVAL-KEY") < 0) {
        fprintf(stderr, "FATAL: License verification failed. Boot aborted.\n");
        return -1;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    uv_loop_t* loop = uv_default_loop();
    uv__queue_init(&g_poll_task_head);
    static uv_signal_t s1, s2;
    uv_signal_init(loop, &s1);
    uv_signal_start(&s1, on_signal_exit, SIGINT);
    uv_signal_init(loop, &s2);
    uv_signal_start(&s2, on_signal_exit, SIGTERM);

    // --- [Deepening] Global Contract Registration ---
    contract_register(&g_contract_manifest);
    const edge_contract_t* contract = contract_get_active();

    g_app.arb = NULL;
    arbiter_init(&g_app.arb, loop);
    g_app.drv_mgr = driver_manager_create();
    edge_driver_registry_init(g_app.drv_mgr);

    const char* shm_name = getenv("EDGE_SHM_NAME") ? getenv("EDGE_SHM_NAME") : EDGE_SHM_BUS_NAME;
    edge_composition_config_t comp_cfg = {.product_name = "IEMS",
                                          .loop = loop,
                                          .shm_name = shm_name,
                                          .ring_capacity = 4096,
                                          .datahub_capacity = 1024,
                                          .tags = contract->tags,
                                          .tag_count = contract->tag_count,
                                          .map_fn = edge_tag_get_compact_index,
                                          .memory_pool_buffer_size = 2048,
                                          .memory_pool_capacity = 64,
                                          .enable_dma_buf = false,
                                          .enable_anti_corruption = true};
    static edge_composition_container_storage_t comp_storage;
    g_app.comp_root = edge_composition_root_create_in(&comp_storage, &comp_cfg);
    g_app.hub = edge_composition_root_get_datahub(g_app.comp_root);

    // [ADR-0001] Start DataHub metrics reporting
    datahub_start_metrics(g_app.hub);
    // 💡 [WAL] 开启 Write-Ahead Log 持久化并重放历史数据
    const char* wal_path =
        getenv("EDGE_WAL_PATH") ? getenv("EDGE_WAL_PATH") : "/tmp/edgeos_wal.log";
    const char* snap_path =
        getenv("EDGE_SNAP_PATH") ? getenv("EDGE_SNAP_PATH") : "/tmp/edgeos_snapshot.bin";

    // 💡 [Compaction] 先加载快照（如果存在），然后重放 WAL 残余日志
    datahub_load_snapshot(g_app.hub, snap_path);

    datahub_enable_wal(g_app.hub, wal_path);
    datahub_replay_wal(g_app.hub);

    // 启动压实调度：每 5 分钟 (300,000ms) 将内存落盘并截断 WAL
    datahub_start_compaction(g_app.hub, 300000, snap_path);

    g_app.orch = bus_orchestrator_create(g_app.hub, g_app.arb, g_app.drv_mgr, 256, 256, 2048);
    datahub_bind_io_pool(g_app.hub, bus_orchestrator_get_pool(g_app.orch));
    edge_api_init(g_app.orch, g_app.drv_mgr);

    for (uint32_t i = 0; i < contract->connectivity_count; i++) {
        const edge_connectivity_t* conn = &contract->connectivity[i];
        const driver_contract_injection_t* inj = contract_model_find("ModbusRTU");

        if (strcmp(conn->category, "SOUTHBOUND") == 0) {
            if (inj)
                bus_orchestrator_setup_master(g_app.orch, conn->id, "ModbusRTU", (uint32_t)conn->id,
                                              inj);
        } else if (strcmp(conn->category, "NORTHBOUND") == 0) {
            if (inj) bus_orchestrator_setup_slave(g_app.orch, conn->id, "ModbusRTU", 1, inj);
        }
    }
    const char* db_env = getenv("EDGE_DB_PATH");
    g_app.db_path = strdup(db_env ? db_env : "iems.db");
    sqlite_config_t db_conf = {.db_path = g_app.db_path, .strategy = STRATEGY_LITE};
    g_app.db_service = sqlite_svc_create(loop, &db_conf);
    g_app.stream_eng =
        stream_engine_create(g_app.hub, NULL, g_app.orch, edge_stage_factory_get_registry());
    pipeline_loader_ctx_t loader_ctx = {
        .db = g_app.db_service,
        .control_ops = {.ctx = g_app.orch, .submit_control = _iems_stream_submit_control},
        .params = NULL};
    stream_engine_load_static_metadata(g_app.stream_eng, contract->pipelines,
                                       contract->pipeline_count, &loader_ctx);
    stream_engine_start(g_app.stream_eng);

    // 💡 [WASM Micro-Runtime] 初始化 WAMR 引擎并导出 Host API
    wasm_mgr_init(g_app.hub);

    g_app.north = calloc(1, sizeof(northbound_mgr_t));

    // 💡 [Golang 原则] 物理实现注入：业务层不知道 MQTT，只拿到一个 transport ops
    const char* mqtt_url = getenv("MQTT_BROKER");
    if (!mqtt_url) mqtt_url = "tcp://localhost:1883";
    nb_transport_ops_t transport = mqtt_producer_create(mqtt_url, "IEMS_EDGE_NODE");

    // 💡 [Consumer-Driven] 创建数据源适配器：datahub_read_ring → nb_source_ops_t
    nb_source_ops_t nb_source = {.ctx = g_app.hub, .read_next = (void*)(datahub_read_ring)};
    northbound_mgr_init_ex(g_app.north, nb_source, "IEMS_V4", g_app.db_service, transport);

    g_app.svc = service_new(loop, g_app.hub, g_app.arb, g_app.db_service, g_app.stream_eng, NULL,
                            g_app.north);
    int rest_port = getenv("EDGE_REST_PORT") ? atoi(getenv("EDGE_REST_PORT")) : 9091;
    rest_server_create(loop, g_app.svc, rest_port);
    g_timer_load_tasks = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, g_timer_load_tasks);
    uv_timer_start(g_timer_load_tasks, iems_load_tasks_timer_cb, 50, 200);
    static uv_timer_t monitor_t;
    uv_timer_init(loop, &monitor_t);
    uv_timer_start(&monitor_t, _monitor_shadow_tick, 1000, 1000);
    static uv_timer_t vt;
    uv_timer_init(loop, &vt);
    g_timer_vbus = &vt;
    uv_timer_start(&vt, _try_connect_buses, 0, 500);
    static uv_timer_t it;
    uv_timer_init(loop, &it);
    g_timer_internal = &it;
    uv_timer_start(&it, _try_connect_internal_buses, 0, 500);

    // [Nexus Fix] 使用宏定义路径，确保契约一致性
    const char* ui_path = EDGE_UI_SOCKET_PATH;
    uv_pipe_t* ui_srv = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, ui_srv, 0);
    unlink(ui_path);
    if (uv_pipe_bind(ui_srv, ui_path) == 0) {
        uv_listen((uv_stream_t*)ui_srv, 128, on_ui_connect);
        printf("\033[32m[IEMS-INIT] UI Companion Socket listening at %s\033[0m\n", ui_path);
    }

    for (uint32_t i = 0; i < contract->connectivity_count; i++) {
        const edge_connectivity_t* c = &contract->connectivity[i];

        if (strcmp(c->category, "NORTHBOUND") == 0) {
            char env_key[32];

            snprintf(env_key, sizeof(env_key), "OMNIS_BUS_%d_DEV", c->id);

            const char* path = getenv(env_key);

            if (!path) path = "/tmp/iems_north_slave.sock";  // [Nexus Fix] 统一测试路径

            if (path) {
                uv_pipe_t* srv = malloc(sizeof(uv_pipe_t));
                uv_pipe_init(loop, srv, 0);
                unlink(path);

                if (uv_pipe_bind(srv, path) == 0 &&
                    uv_listen((uv_stream_t*)srv, 128, _on_north_connect) == 0) {
                    north_accept_ctx_t* ctx = malloc(sizeof(north_accept_ctx_t));

                    ctx->bus_id = c->id;
                    ctx->bus_idx = c->id % MAX_VBUS;
                    srv->data = ctx;

                    g_bus_state[ctx->bus_idx] = BUS_STATE_LISTENING;

                    printf("\033[32m[IEMS-INIT] Northbound Slave LISTENING at %s\033[0m\n", path);

                } else
                    uv_close((uv_handle_t*)srv, (uv_close_cb)free);
            }
        }
    }
    arbiter_resume(g_app.arb);
    service_run(g_app.svc);
    iems_shutdown(loop);
    return 0;
}
