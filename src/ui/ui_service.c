#include <stdlib.h>
#include <unistd.h>

#include "edge_log.h"
#include "edge_ui.h"

#ifdef UI_DRIVER_NCURSES
#include "ui_driver_ncurses.h"
#endif

ui_service_t *ui_service_create(datahub_t *hub) {
    ui_service_t *service = calloc(1, sizeof(ui_service_t));
    if (!service) {
        return NULL;
    }
    service->hub = hub;
    service->running = false;
    service->stack_top = -1;

#ifdef UI_DRIVER_NCURSES
    service->hal = ui_hal_ncurses_create();
#endif

    if (service->hal && service->hal->init) {
        service->hal->init(service->hal);
    } else {
        LOG_WARN(LOG_CAT_UI, "No UI HAL driver initialized!");
    }

    return service;
}

void ui_service_destroy(ui_service_t *service) {
    if (!service) {
        return;
    }

    if (service->hal) {
        if (service->hal->destroy) service->hal->destroy(service->hal);
        free(service->hal->priv);
        free(service->hal);
    }

    free(service);
}

void ui_service_run(ui_service_t *service) {
    if (!service) {
        return;
    }

    if (ui_product_init) {
        ui_product_init(service);
    }

    if (service->stack_top < 0) {
        LOG_ERROR(LOG_CAT_UI, "No pages registered. UI cannot start.");
        return;
    }

    service->running = true;

    while (service->running) {
        ui_page_t *current_page = service->page_stack[service->stack_top];

        // 1. 处理输入
        if (service->hal && service->hal->get_key) {
            int key = service->hal->get_key(service->hal);
            if (key != -1 && current_page->on_input) {
                current_page->on_input(current_page, service, key);
            }
        }

        // 2. 渲染绘制
        if (service->hal && service->hal->clear) {
            service->hal->clear(service->hal);
        }

        if (current_page->on_draw) {
            current_page->on_draw(current_page, service);
        }

        if (service->hal && service->hal->refresh) {
            service->hal->refresh(service->hal);
        }

        usleep(50 * 1000);  // 提高刷新率到 20Hz 增强按键响应
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
