#ifndef EDGE_UI_H
#define EDGE_UI_H

#include <stdbool.h>
#include <uv.h>

#include "datahub.h"

// Forward declarations
typedef struct ui_service_s ui_service_t;
typedef struct ui_page_s ui_page_t;
typedef struct ui_hal_s ui_hal_t;

/**
 * @brief UI Hardware Abstraction Layer (HAL)
 */
struct ui_hal_s {
    const char* name;
    void (*hal_init)(ui_hal_t* hal);
    void (*hal_clear)(ui_hal_t* hal);
    void (*hal_draw_text)(ui_hal_t* hal, int x, int y, const char* str, int color);
    void (*hal_draw_pixel)(ui_hal_t* hal, int x, int y, int color);
    void (*hal_draw_box)(ui_hal_t* hal, int x, int y, int w, int h);
    int (*hal_get_key)(ui_hal_t* hal);
    void (*hal_refresh)(ui_hal_t* hal);
    void (*hal_destroy)(ui_hal_t* hal);
    void* priv;
};

// UI Colors
#define UI_COLOR_DEFAULT 0
#define UI_COLOR_RED 1
#define UI_COLOR_GREEN 2
#define UI_COLOR_BLUE 3
#define UI_COLOR_YELLOW 4
#define UI_COLOR_CYAN 5

// Page function pointers
typedef void (*ui_page_on_enter)(ui_page_t* page, ui_service_t* service);
typedef void (*ui_page_on_exit)(ui_page_t* page, ui_service_t* service);
typedef void (*ui_page_on_draw)(ui_page_t* page, ui_service_t* service);
typedef void (*ui_page_on_input)(ui_page_t* page, ui_service_t* service, int key);

struct ui_page_s {
    const char* name;
    void* user_data;
    ui_page_on_enter on_enter;
    ui_page_on_exit on_exit;
    ui_page_on_draw on_draw;
    ui_page_on_input on_input;
};

// Main UI service structure
struct ui_service_s {
    bool running;
    datahub_t* hub;
    uv_loop_t* loop;  // Need to store the loop for the timer
    ui_hal_t* hal;
    uv_timer_t refresh_timer;
    ui_page_t* page_stack[8];
    int stack_top;
};

// Service functions
ui_service_t* ui_service_create(datahub_t* hub);
void ui_service_destroy(ui_service_t* service);
void ui_service_run(ui_service_t* service);
void ui_service_stop(ui_service_t* service);

// Page stack management
void ui_service_push_page(ui_service_t* service, ui_page_t* page);
void ui_service_pop_page(ui_service_t* service);

// HAL Creators
ui_hal_t* ui_hal_ncurses_create(void);

/**
 * @brief Product-specific UI initialization function.
 *
 * This is a weak function that should be implemented by the
 * product-specific UI code to register its initial pages.
 *
 * @param service The UI service instance.
 */
__attribute__((weak)) void ui_product_init(ui_service_t* service);

#endif  // EDGE_UI_H
