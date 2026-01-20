#ifndef EDGE_UI_H
#define EDGE_UI_H

#include <stdbool.h>
#include "datahub.h"

// Forward declarations
typedef struct ui_service_s ui_service_t;
typedef struct ui_page_s ui_page_t;

// Page function pointers
typedef void (*ui_page_on_enter)(ui_page_t *page, ui_service_t *service);
typedef void (*ui_page_on_exit)(ui_page_t *page, ui_service_t *service);
typedef void (*ui_page_on_draw)(ui_page_t *page, ui_service_t *service);
typedef void (*ui_page_on_input)(ui_page_t *page, ui_service_t *service, int key);

struct ui_page_s {
    const char *name;
    void *user_data;
    ui_page_on_enter on_enter;
    ui_page_on_exit on_exit;
    ui_page_on_draw on_draw;
    ui_page_on_input on_input;
};

// Main UI service structure
struct ui_service_s {
    bool running;
    datahub_t *hub;
    ui_page_t *page_stack[8];
    int stack_top;
};

// Service functions
ui_service_t* ui_service_create(datahub_t *hub);
void ui_service_destroy(ui_service_t *service);
void ui_service_run(ui_service_t *service);
void ui_service_stop(ui_service_t *service);

// Page stack management
void ui_service_push_page(ui_service_t *service, ui_page_t *page);
void ui_service_pop_page(ui_service_t *service);

/**
 * @brief Product-specific UI initialization function.
 * 
 * This is a weak function that should be implemented by the
 * product-specific UI code to register its initial pages.
 * 
 * @param service The UI service instance.
 */
__attribute__((weak)) void ui_product_init(ui_service_t *service);

#endif // EDGE_UI_H
