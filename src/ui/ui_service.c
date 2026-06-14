#include <stdlib.h>
#include <unistd.h>

#include "edge_log.h"
#include "edge_ui.h"

// [UI-01] New timer callback for asynchronous refresh
static void _ui_on_refresh(uv_timer_t* handle) {
    ui_service_t* service = (ui_service_t*)handle->data;
    if (!service->running || service->stack_top < 0) {
        return;
    }

    ui_page_t* current_page = service->page_stack[service->stack_top];

    // 1. Process input
    if (service->hal && service->hal->hal_get_key) {
        int key = service->hal->hal_get_key(service->hal);
        if (key != -1) {
            // Global key handling for ESC to pop a page
            if (key == 27) {  // 27 is the ASCII code for Escape
                ui_service_pop_page(service);
                // If stack becomes empty after pop, stop the service
                if (service->stack_top < 0) {
                    ui_service_stop(service);
                    return;  // Stop processing this frame
                }
            } else if (current_page->on_input) {
                // Page-specific input
                current_page->on_input(current_page, service, key);
            }
        }
    }

    // 2. Render
    if (service->hal && service->hal->hal_clear) {
        service->hal->hal_clear(service->hal);
    }

    if (current_page && current_page->on_draw) {
        current_page->on_draw(current_page, service);
    }

    if (service->hal && service->hal->hal_refresh) {
        service->hal->hal_refresh(service->hal);
    }
}

// [UI-02] create is now lean, no HAL is passed in.
ui_service_t* ui_service_create(datahub_t* hub) {
    ui_service_t* service = calloc(1, sizeof(ui_service_t));
    if (!service) {
        return NULL;
    }
    service->hub = hub;
    service->loop = hub->loop;  // Get loop from hub
    service->hal = NULL;        // HAL will be injected by product_init
    service->running = false;
    service->stack_top = -1;

    // Initialize the timer, but don't start it yet.
    uv_timer_init(service->loop, &service->refresh_timer);
    service->refresh_timer.data = service;

    return service;
}

void ui_service_destroy(ui_service_t* service) {
    if (!service) {
        return;
    }

    if (!uv_is_closing((uv_handle_t*)&service->refresh_timer)) {
        uv_close((uv_handle_t*)&service->refresh_timer, NULL);
    }

    if (service->hal) {
        if (service->hal->hal_destroy) service->hal->hal_destroy(service->hal);
        if (service->hal->priv) free(service->hal->priv);
        free(service->hal);
    }

    free(service);
}

// [UI-01] run is now non-blocking and orchestrates init.
void ui_service_run(ui_service_t* service) {
    if (!service) {
        return;
    }

    // [UI-07] Product code is called first to inject HAL and pages.
    if (ui_product_init) {
        ui_product_init(service);
    }

    // Now that product_init has run, check if a HAL was injected.
    if (!service->hal) {
        LOG_ERROR(LOG_CAT_UI, "ui_product_init did not inject a HAL driver. UI cannot start.");
        return;
    }

    // Initialize the newly injected driver.
    if (service->hal->hal_init) {
        service->hal->hal_init(service->hal);
    }

    if (service->stack_top < 0) {
        LOG_ERROR(LOG_CAT_UI, "No pages registered by ui_product_init. UI cannot start.");
        return;
    }

    service->running = true;
    // Start the asynchronous refresh loop (20Hz).
    uv_timer_start(&service->refresh_timer, _ui_on_refresh, 50, 50);
    uv_run(service->loop, UV_RUN_DEFAULT);
}

void ui_service_stop(ui_service_t* service) {
    if (service && service->running) {
        service->running = false;
        uv_timer_stop(&service->refresh_timer);
        // [整改] 强制停止 libuv 循环，确保 uv_run 立即返回
        if (service->loop) {
            uv_stop(service->loop);
        }
    }
}

// [UI-03] Page stack logic with on_enter/on_exit hooks
void ui_service_push_page(ui_service_t* service, ui_page_t* page) {
    if (!service || service->stack_top >= 7) {
        LOG_ERROR(LOG_CAT_UI, "Cannot push page, stack is full.");
        return;
    }

    if (service->stack_top >= 0 && service->page_stack[service->stack_top]->on_exit) {
        service->page_stack[service->stack_top]->on_exit(service->page_stack[service->stack_top],
                                                         service);
    }

    service->stack_top++;
    service->page_stack[service->stack_top] = page;

    if (page && page->on_enter) {
        page->on_enter(page, service);
    }
}

void ui_service_pop_page(ui_service_t* service) {
    if (!service || service->stack_top < 0) {
        return;
    }

    if (service->page_stack[service->stack_top]->on_exit) {
        service->page_stack[service->stack_top]->on_exit(service->page_stack[service->stack_top],
                                                         service);
    }

    service->stack_top--;

    if (service->stack_top >= 0 && service->page_stack[service->stack_top]->on_enter) {
        service->page_stack[service->stack_top]->on_enter(service->page_stack[service->stack_top],
                                                          service);
    }
}

__attribute__((weak)) void ui_product_init(ui_service_t* service) {
    (void)service;
    LOG_INFO(LOG_CAT_UI, "Default weak ui_product_init() called. No pages loaded.");
}
