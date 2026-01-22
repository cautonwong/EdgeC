#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "edge_log.h"
#include "edge_ui.h"

extern const edge_tag_meta_t g_tag_metadata[];
extern const uint32_t g_tag_metadata_len;

typedef enum { PAGE_DASHBOARD = 0, PAGE_IO_DETAILS = 1, PAGE_COUNT } iems_page_idx_t;
static iems_page_idx_t g_current_page = PAGE_DASHBOARD;

static void draw_frame(ui_hal_t *hal, const char *title) {
    hal->draw_box(hal, 0, 0, 50, 15);
    char buf[64];
    snprintf(buf, sizeof(buf), "--- %s ---", title);
    hal->draw_text(hal, 15, 1, buf, UI_COLOR_CYAN);
    hal->draw_text(hal, 2, 14, "[F1]Dashboard [F2]Inventory [Q]Quit", UI_COLOR_BLUE);
}

static void render_dashboard(ui_hal_t *hal, datahub_t *hub) {
    draw_frame(hal, "IEMS NODE");
    for (uint32_t i = 0; i < g_tag_metadata_len && i < 5; i++) {
        pipe_msg_t msg = {0};
        double val = 0.0;
        if (datahub_read_shadow(hub, g_tag_metadata[i].id, &msg) == 0) {
            val = msg.payload.val.f;
        }
        char line[128];
        snprintf(line, sizeof(line), "%-20.20s : %8.2f %s", g_tag_metadata[i].name, val,
                 g_tag_metadata[i].unit ? g_tag_metadata[i].unit : "");
        hal->draw_text(hal, 4, 4 + (int)i, line, (val > 0) ? UI_COLOR_GREEN : UI_COLOR_DEFAULT);
    }
}

static void dashboard_on_draw(ui_page_t *page, ui_service_t *service) {
    (void)page;
    ui_hal_t *hal = service->hal;
    if (!hal) return;
    if (g_current_page == PAGE_DASHBOARD)
        render_dashboard(hal, service->hub);
    else
        draw_frame(hal, "INVENTORY");
}

static void dashboard_on_input(ui_page_t *page, ui_service_t *service, int key) {
    (void)page;
    if (key == KEY_F(1))
        g_current_page = PAGE_DASHBOARD;
    else if (key == KEY_F(2))
        g_current_page = PAGE_IO_DETAILS;
    else if (key == 'q' || key == 'Q')
        ui_service_stop(service);
}

static ui_page_t g_dashboard_page = {
    .name = "Main",
    .on_draw = dashboard_on_draw,
    .on_input = dashboard_on_input,
};

// [UI-07] This is the strong implementation that overrides the weak one in the framework.
// It creates the specific driver and injects it into the service.
void ui_product_init(ui_service_t *service) {
    // 1. Create and inject the ncurses driver
    ui_hal_t *hal = ui_hal_ncurses_create();
    service->hal = hal;

    // 2. Push the product-specific page(s)
    ui_service_push_page(service, &g_dashboard_page);
}
