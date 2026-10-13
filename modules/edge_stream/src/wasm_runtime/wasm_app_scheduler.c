#include "wasm_app_scheduler.h"
#include <uv.h>
#include <stdlib.h>
#include <string.h>
#include "edge_log.h"

#define MAX_SCHEDULED_APPS 16

typedef struct {
    char name[64];
    wasm_app_t *app;
    bool active;
    bool in_use;
} wasm_scheduled_app_t;

struct wasm_scheduler_s {
    datahub_t *hub;
    uv_loop_t *loop;
    uv_timer_t timer;
    uint32_t tick_period_ms;
    wasm_scheduled_app_t apps[MAX_SCHEDULED_APPS];
    uv_mutex_t lock;
    bool running;
    bool is_static;
};

static void wasm_scheduler_tick(uv_timer_t *timer) {
    wasm_scheduler_t *sched = (wasm_scheduler_t *)timer->data;
    if (!sched) return;

    uv_mutex_lock(&sched->lock);
    for (int i = 0; i < MAX_SCHEDULED_APPS; i++) {
        wasm_scheduled_app_t *curr = &sched->apps[i];
        if (curr->in_use && curr->active && curr->app) {
            LOG_INFO(LOG_CAT_CORE, "WASM Scheduler: Ticking app '%s'", curr->name);
            // Execute on_tick
            int rc = wasm_app_exec(curr->app, "on_tick", NULL);
            if (rc != 0) {
                LOG_ERROR(LOG_CAT_CORE, "WASM Scheduler: App '%s' returned non-zero or failed", curr->name);
            }
        }
    }
    uv_mutex_unlock(&sched->lock);
}

static struct wasm_scheduler_s g_wasm_scheduler;

wasm_scheduler_t *wasm_scheduler_create(datahub_t *hub, uv_loop_t *loop, uint32_t tick_period_ms) {
    if (!hub || !loop) return NULL;

    wasm_scheduler_t *sched = &g_wasm_scheduler;
    memset(sched, 0, sizeof(*sched));

    sched->hub = hub;
    sched->loop = loop;
    sched->tick_period_ms = tick_period_ms > 0 ? tick_period_ms : 1000;
    sched->running = false;
    sched->is_static = true;

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
    for (int i = 0; i < MAX_SCHEDULED_APPS; i++) {
        wasm_scheduled_app_t *curr = &sched->apps[i];
        if (curr->in_use && curr->app) {
            wasm_app_destroy(curr->app);
            curr->app = NULL;
        }
        curr->in_use = false;
        curr->active = false;
    }
    uv_mutex_unlock(&sched->lock);

    uv_mutex_destroy(&sched->lock);
    uv_close((uv_handle_t *)&sched->timer, on_timer_close);
    
    if (!sched->is_static) {
        free(sched);
    }
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
    
    int free_slot = -1;
    // Check if app name already exists and find free slot
    for (int i = 0; i < MAX_SCHEDULED_APPS; i++) {
        if (sched->apps[i].in_use && strcmp(sched->apps[i].name, name) == 0) {
            LOG_ERROR(LOG_CAT_CORE, "WASM Scheduler: App '%s' already loaded", name);
            uv_mutex_unlock(&sched->lock);
            return -1;
        }
        if (!sched->apps[i].in_use && free_slot < 0) {
            free_slot = i;
        }
    }

    if (free_slot < 0) {
        LOG_ERROR(LOG_CAT_CORE, "WASM Scheduler: Maximum app limit (%d) reached", MAX_SCHEDULED_APPS);
        uv_mutex_unlock(&sched->lock);
        return -1;
    }

    // Create WASM container
    wasm_app_t *wasm_app = wasm_app_create(wasm_path, heap_size, stack_size, timeout_ms);
    if (!wasm_app) {
        LOG_ERROR(LOG_CAT_CORE, "WASM Scheduler: Failed to create app '%s' from path '%s'", name, wasm_path);
        uv_mutex_unlock(&sched->lock);
        return -1;
    }

    wasm_scheduled_app_t *slot = &sched->apps[free_slot];
    memset(slot, 0, sizeof(*slot));
    strncpy(slot->name, name, sizeof(slot->name) - 1);
    slot->app = wasm_app;
    slot->active = true;
    slot->in_use = true;

    LOG_INFO(LOG_CAT_CORE, "WASM Scheduler: Loaded and scheduled app '%s' successfully", name);
    uv_mutex_unlock(&sched->lock);
    return 0;
}

int wasm_scheduler_unload_app(wasm_scheduler_t *sched, const char *name) {
    if (!sched || !name) return -1;

    uv_mutex_lock(&sched->lock);
    for (int i = 0; i < MAX_SCHEDULED_APPS; i++) {
        wasm_scheduled_app_t *curr = &sched->apps[i];
        if (curr->in_use && strcmp(curr->name, name) == 0) {
            if (curr->app) {
                wasm_app_destroy(curr->app);
                curr->app = NULL;
            }
            curr->in_use = false;
            curr->active = false;
            LOG_INFO(LOG_CAT_CORE, "WASM Scheduler: Unloaded and destroyed app '%s'", name);
            uv_mutex_unlock(&sched->lock);
            return 0;
        }
    }

    LOG_WARN(LOG_CAT_CORE, "WASM Scheduler: App '%s' not found to unload", name);
    uv_mutex_unlock(&sched->lock);
    return -1;
}

int wasm_scheduler_reload_app(wasm_scheduler_t *sched, const char *name, const char *wasm_path, uint32_t heap_size, uint32_t stack_size, uint32_t timeout_ms) {
    if (!sched || !name || !wasm_path) return -1;

    uv_mutex_lock(&sched->lock);
    
    wasm_scheduled_app_t *curr = NULL;
    for (int i = 0; i < MAX_SCHEDULED_APPS; i++) {
        if (sched->apps[i].in_use && strcmp(sched->apps[i].name, name) == 0) {
            curr = &sched->apps[i];
            break;
        }
    }

    if (!curr) {
        LOG_ERROR(LOG_CAT_CORE, "WASM Scheduler: App '%s' not found for reload", name);
        uv_mutex_unlock(&sched->lock);
        return -1;
    }

    // Create new WASM container first to ensure it's valid before discarding the old one
    wasm_app_t *new_wasm_app = wasm_app_create(wasm_path, heap_size, stack_size, timeout_ms);
    if (!new_wasm_app) {
        LOG_ERROR(LOG_CAT_CORE, "WASM Scheduler: Failed to create new app container for reload of '%s'", name);
        uv_mutex_unlock(&sched->lock);
        return -1;
    }

    // Destroy old container
    if (curr->app) {
        wasm_app_destroy(curr->app);
    }
    
    curr->app = new_wasm_app;
    curr->active = true;

    LOG_INFO(LOG_CAT_CORE, "WASM Scheduler: App '%s' hot-reloaded successfully", name);
    uv_mutex_unlock(&sched->lock);
    return 0;
}
