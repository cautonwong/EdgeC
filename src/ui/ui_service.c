#include "edge_ui.h"
#include "edge_log.h"
#include <stdlib.h>
#include <unistd.h>

#ifdef UI_DRIVER_NCURSES
#include "ui_driver_ncurses.h"
#endif

ui_service_t* ui_service_create(datahub_t *hub) {
    ui_service_t *service = calloc(1, sizeof(ui_service_t));
    if (!service) {
        return NULL;
    }
    service->hub = hub;
    service->running = false;
    service->stack_top = -1;
    
#ifdef UI_DRIVER_NCURSES
    ui_driver_ncurses_init();
#else
    LOG_WARN(LOG_CAT_UI, "No UI driver specified!");
#endif

    return service;
}

void ui_service_destroy(ui_service_t *service) {
    if (!service) {
        return;
    }

#ifdef UI_DRIVER_NCURSES
    ui_driver_ncurses_teardown();
#endif

    free(service);
}

void ui_service_run(ui_service_t *service) {
    if (!service) {
        return;
    }
    
    // Call the product-specific initializer if it exists
    if (ui_product_init) {
        ui_product_init(service);
    }

    if (service->stack_top < 0) {
        LOG_ERROR(LOG_CAT_UI, "No pages registered. UI cannot start.");
        return;
    }
    
    service->running = true;
    
    ui_page_t *current_page = service->page_stack[service->stack_top];
    if (current_page->on_enter) {
        current_page->on_enter(current_page, service);
    }

    while (service->running) {
        if (current_page->on_draw) {
            current_page->on_draw(current_page, service);
        }

        // A real implementation would use select() or poll() on stdin
        // and the datahub's notification fd. This is a simplification.
        usleep(1000 * 100); // 100ms
    }
}

void ui_service_stop(ui_service_t *service) {
    if (service) {
        service->running = false;
    }
}

void ui_service_push_page(ui_service_t *service, ui_page_t *page) {
    if (!service || service->stack_top >= 7) {
        LOG_ERROR(LOG_CAT_UI, "Cannot push page, stack is full.");
        return;
    }
    
    service->stack_top++;
    service->page_stack[service->stack_top] = page;
}

void ui_service_pop_page(ui_service_t *service) {
    if (!service || service->stack_top < 0) {
        LOG_ERROR(LOG_CAT_UI, "Cannot pop page, stack is empty.");
        return;
    }
    
    service->stack_top--;
}

// Default weak implementation of the product initializer
__attribute__((weak)) void ui_product_init(ui_service_t *service) {
    (void)service;
    // This function can be overridden by product-specific code
    LOG_INFO(LOG_CAT_UI, "Default weak ui_product_init() called. No pages loaded.");
}
