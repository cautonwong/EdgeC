#include "services/script_loader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#define MAX_SCRIPTS 32

struct script_loader_s {
    js_engine_t *engine;
    char dir_path[256];
    uint64_t interval;
    
    uv_timer_t timer;
    uv_fs_event_t fs_event;
    
    char *scripts[MAX_SCRIPTS];
    int script_count;
};

// --- Helper: Scan directory for .js files ---
static void scan_scripts(script_loader_t *loader) {
    printf("[Loader] Scanning directory: %s\n", loader->dir_path);
    
    // Clear old list
    for (int i = 0; i < loader->script_count; i++) {
        free(loader->scripts[i]);
    }
    loader->script_count = 0;

    DIR *d = opendir(loader->dir_path);
    if (!d) {
        perror("opendir");
        return;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strstr(dir->d_name, ".js")) {
            if (loader->script_count < MAX_SCRIPTS) {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s/%s", loader->dir_path, dir->d_name);
                loader->scripts[loader->script_count++] = strdup(full_path);
                printf("[Loader] Found script: %s\n", dir->d_name);
            }
        }
    }
    closedir(d);
}

// --- Timer Callback: Execute all scripts ---
static void on_timer(uv_timer_t *handle) {
    script_loader_t *loader = (script_loader_t *)handle->data;
    
    for (int i = 0; i < loader->script_count; i++) {
        // printf("[Loader] Executing %s...\n", loader->scripts[i]);
        js_engine_run_file(loader->engine, loader->scripts[i]);
    }
}

// --- FS Event Callback: Reload on change ---
static void on_fs_event(uv_fs_event_t *handle, const char *filename, int events, int status) {
    (void)events; (void)status;
    script_loader_t *loader = (script_loader_t *)handle->data;
    printf("[Loader] Directory change detected (%s), reloading scripts...\n", filename);
    scan_scripts(loader);
}

script_loader_t* script_loader_create(js_engine_t *engine, const char *dir_path, uint64_t interval_ms) {
    script_loader_t *loader = calloc(1, sizeof(script_loader_t));
    if (!loader) return NULL;

    loader->engine = engine;
    strncpy(loader->dir_path, dir_path, sizeof(loader->dir_path) - 1);
    loader->interval = interval_ms;

    uv_loop_t *loop = js_engine_get_loop(engine);
    uv_timer_init(loop, &loader->timer);
    loader->timer.data = loader;

    uv_fs_event_init(loop, &loader->fs_event);
    loader->fs_event.data = loader;

    return loader;
}

int script_loader_start(script_loader_t *loader) {
    // 1. Initial scan
    scan_scripts(loader);

    // 2. Start execution timer
    uv_timer_start(&loader->timer, on_timer, 0, loader->interval);

    // 3. Start watching directory
    return uv_fs_event_start(&loader->fs_event, on_fs_event, loader->dir_path, 0);
}

void script_loader_destroy(script_loader_t *loader) {
    if (!loader) return;
    uv_timer_stop(&loader->timer);
    uv_fs_event_stop(&loader->fs_event);
    
    for (int i = 0; i < loader->script_count; i++) {
        free(loader->scripts[i]);
    }
    free(loader);
}
