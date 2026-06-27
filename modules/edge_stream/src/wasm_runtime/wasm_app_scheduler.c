#include "wasm_app_scheduler.h"
#include <stdlib.h>
#include <string.h>
#include "edge_log.h"

typedef struct wasm_scheduled_app_s {
    char name[64];
    wasm_app_t *app;
    bool active;
    struct wasm_scheduled_app_s *next;
} wasm_scheduled_app_t;

struct wasm_scheduler_s {
    datahub_t *hub;
    uv_loop_t *loop;
    uv_timer_t timer;
    uint32_t tick_period_ms;
    wasm_scheduled_app_t *apps_head;
    uv_mutex_t lock;
    bool running;
};

static void wasm_scheduler_tick(uv_timer_t *timer) {
    wasm_scheduler_t *sched = (wasm_scheduler_t *)timer->data;
    if (!sched) return;

    uv_mutex_lock(&sched->lock);
    wasm_scheduled_app_t *curr = sched->apps_head;
    while (curr) {
        if (curr->active && curr->app) {
            LOG_INFO(LOG_CAT_CORE, "WASM Scheduler: Ticking app '%s'", curr->name);
            // Execute on_tick
            int rc = wasm_app_exec(curr->app, "on_tick", NULL);
            if (rc != 0) {
                LOG_ERROR(LOG_CAT_CORE, "WASM Scheduler: App '%s' returned non-zero or failed", curr->name);
            }
        }
        curr = curr->next;
    }
    uv_mutex_unlock(&sched->lock);
}

wasm_scheduler_t *wasm_scheduler_create(datahub_t *hub, uv_loop_t *loop, uint32_t tick_period_ms) {
    if (!hub || !loop) return NULL;

    wasm_scheduler_t *sched = calloc(1, sizeof(wasm_scheduler_t));
    if (!sched) return NULL;

    sched->hub = hub;
    sched->loop = loop;
    sched->tick_period_ms = tick_period_ms > 0 ? tick_period_ms : 1000;
    sched->apps_head = NULL;
    sched->running = false;

    uv_mutex_init(&sched->lock);
    uv_timer_init(loop, &sched->timer);
    sched->timer.data = sched;

    LOG_INFO(LOG_CAT_CORE, "WASM Scheduler: Created with period %u ms", sched->tick_period_ms);
    return sched;
}

static void on_timer_close(uv_handle_t *handle) {
    (void)handle;
}

void wasm_scheduler_destroy(wasm_scheduler_t *sched) {
    if (!sched) return;

    wasm_scheduler_stop(sched);

    // Destroy all scheduled apps
    uv_mutex_lock(&sched->lock);
    wasm_scheduled_app_t *curr = sched->apps_head;
    while (curr) {
        wasm_scheduled_app_t *next = curr->next;
        if (curr->app) {
            wasm_app_destroy(curr->app);
        }
        free(curr);
        curr = next;
    }
    sched->apps_head = NULL;
    uv_mutex_unlock(&sched->lock);

    uv_mutex_destroy(&sched->lock);
    uv_close((uv_handle_t *)&sched->timer, on_timer_close);
    
    // Allow loop to run close callback
    free(sched);
    LOG_INFO(LOG_CAT_CORE, "WASM Scheduler: Destroyed");
}

int wasm_scheduler_start(wasm_scheduler_t *sched) {
    if (!sched) return -1;

    uv_mutex_lock(&sched->lock);
    if (sched->running) {
        uv_mutex_unlock(&sched->lock);
        return 0;
    }

    int rc = uv_timer_start(&sched->timer, wasm_scheduler_tick, sched->tick_period_ms, sched->tick_period_ms);
    if (rc == 0) {
        sched->running = true;
        LOG_INFO(LOG_CAT_CORE, "WASM Scheduler: Started");
    } else {
        LOG_ERROR(LOG_CAT_CORE, "WASM Scheduler: Failed to start timer: %s", uv_strerror(rc));
    }
    uv_mutex_unlock(&sched->lock);
    return rc == 0 ? 0 : -1;
}

int wasm_scheduler_stop(wasm_scheduler_t *sched) {
    if (!sched) return -1;

    uv_mutex_lock(&sched->lock);
    if (!sched->running) {
        uv_mutex_unlock(&sched->lock);
        return 0;
    }

    int rc = uv_timer_stop(&sched->timer);
    if (rc == 0) {
        sched->running = false;
        LOG_INFO(LOG_CAT_CORE, "WASM Scheduler: Stopped");
    } else {
        LOG_ERROR(LOG_CAT_CORE, "WASM Scheduler: Failed to stop timer: %s", uv_strerror(rc));
    }
    uv_mutex_unlock(&sched->lock);
    return rc == 0 ? 0 : -1;
}

int wasm_scheduler_load_app(wasm_scheduler_t *sched, const char *name, const char *wasm_path, uint32_t heap_size, uint32_t stack_size, uint32_t timeout_ms) {
    if (!sched || !name || !wasm_path) return -1;

    uv_mutex_lock(&sched->lock);
    
    // Check if app name already exists
    wasm_scheduled_app_t *curr = sched->apps_head;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            LOG_ERROR(LOG_CAT_CORE, "WASM Scheduler: App '%s' already loaded", name);
            uv_mutex_unlock(&sched->lock);
            return -1;
        }
        curr = curr->next;
    }

    // Create WASM container
    wasm_app_t *wasm_app = wasm_app_create(wasm_path, heap_size, stack_size, timeout_ms);
    if (!wasm_app) {
        LOG_ERROR(LOG_CAT_CORE, "WASM Scheduler: Failed to create app '%s' from path '%s'", name, wasm_path);
        uv_mutex_unlock(&sched->lock);
        return -1;
    }

    // Add to scheduled list
    wasm_scheduled_app_t *new_node = calloc(1, sizeof(wasm_scheduled_app_t));
    if (!new_node) {
        wasm_app_destroy(wasm_app);
        uv_mutex_unlock(&sched->lock);
        return -1;
    }

    strncpy(new_node->name, name, sizeof(new_node->name) - 1);
    new_node->app = wasm_app;
    new_node->active = true;
    new_node->next = sched->apps_head;
    sched->apps_head = new_node;

    LOG_INFO(LOG_CAT_CORE, "WASM Scheduler: Loaded and scheduled app '%s' successfully", name);
    uv_mutex_unlock(&sched->lock);
    return 0;
}

int wasm_scheduler_unload_app(wasm_scheduler_t *sched, const char *name) {
    if (!sched || !name) return -1;

    uv_mutex_lock(&sched->lock);
    wasm_scheduled_app_t *prev = NULL;
    wasm_scheduled_app_t *curr = sched->apps_head;

    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            // Remove from list
            if (prev) {
                prev->next = curr->next;
            } else {
                sched->apps_head = curr->next;
            }

            // Destroy container
            if (curr->app) {
                wasm_app_destroy(curr->app);
            }
            free(curr);
            LOG_INFO(LOG_CAT_CORE, "WASM Scheduler: Unloaded and destroyed app '%s'", name);
            uv_mutex_unlock(&sched->lock);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    LOG_WARN(LOG_CAT_CORE, "WASM Scheduler: App '%s' not found to unload", name);
    uv_mutex_unlock(&sched->lock);
    return -1;
}
