#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "edge_log.h"
#include "edge_ui.h"

extern const edge_tag_meta_t g_tag_metadata[];
extern const uint32_t g_tag_metadata_len;

typedef enum {
    PAGE_DASHBOARD = 0,
    PAGE_IO_DETAILS = 1,
    PAGE_ALARM = 2,
    PAGE_COUNT
} iems_page_idx_t;

typedef struct {
    iems_page_idx_t current_page;
    bool datahub_connected;
    uint32_t active_tag_v;
    uint32_t active_tag_i;
} iems_ui_state_t;

static iems_ui_state_t g_ui_state = {.current_page = PAGE_DASHBOARD,
                                     .datahub_connected = false,
                                     .active_tag_v = 0,
                                     .active_tag_i = 0};

static void draw_common_frame(ui_hal_t *hal, const char *title) {
    hal->draw_box(hal, 0, 0, 50, 15);
    hal->draw_text(hal, 2, 0, " [ ", UI_COLOR_DEFAULT);
    hal->draw_text(hal, 5, 0, title, UI_COLOR_YELLOW);
    hal->draw_text(hal, 5 + (int)strlen(title), 0, " ] ", UI_COLOR_DEFAULT);
    hal->draw_text(hal, 2, 14, " F1:Main  F2:IO  F3:Alm  Q:Exit ", UI_COLOR_BLUE);
}

static void render_dashboard(ui_hal_t *hal, datahub_t *hub) {
    draw_common_frame(hal, "DASHBOARD");

    // 如果还没找到有效 Tag，尝试初始化寻找
    if (g_ui_state.active_tag_v == 0 && g_tag_metadata_len > 0) {
        g_ui_state.active_tag_v = g_tag_metadata[0].id;
        if (g_tag_metadata_len > 1) g_ui_state.active_tag_i = g_tag_metadata[1].id;
    }

    pipe_msg_t msg_v = {0}, msg_i = {0};
    bool v_ok = (datahub_read_shadow(hub, g_ui_state.active_tag_v, &msg_v) == 0);
    bool i_ok = (datahub_read_shadow(hub, g_ui_state.active_tag_i, &msg_i) == 0);

    if (!v_ok) {
        hal->draw_text(hal, 10, 7, "WAITING FOR LIVE DATA...", UI_COLOR_RED);
        char buf[64];
        snprintf(buf, sizeof(buf), "(Polling Tag ID: %u)", g_ui_state.active_tag_v);
        hal->draw_text(hal, 12, 9, buf, UI_COLOR_DEFAULT);
    } else {
        char buf[64];
        hal->draw_text(hal, 5, 4, "Primary Value:", UI_COLOR_DEFAULT);
        snprintf(buf, sizeof(buf), "%.2f", msg_v.payload.val.f);
        hal->draw_text(hal, 22, 4, buf, UI_COLOR_GREEN);

        if (i_ok) {
            hal->draw_text(hal, 5, 6, "Secondary Value:", UI_COLOR_DEFAULT);
            snprintf(buf, sizeof(buf), "%.2f", msg_i.payload.val.f);
            hal->draw_text(hal, 22, 6, buf, UI_COLOR_GREEN);
        }

        hal->draw_text(hal, 5, 12, "DataHub Status:", UI_COLOR_DEFAULT);
        hal->draw_text(hal, 22, 12, "CONNECTED / LIVE", UI_COLOR_BLUE);
    }
}

static void render_io_details(ui_hal_t *hal, datahub_t *hub) {
    (void)hub;
    draw_common_frame(hal, "TAG INVENTORY");
    hal->draw_text(hal, 4, 2, "ID       |   NAME           |  VALUE", UI_COLOR_BLUE);
    hal->draw_text(hal, 4, 3, "---------+------------------+-------", UI_COLOR_DEFAULT);

    for (uint32_t i = 0; i < g_tag_metadata_len && i < 8; i++) {
        pipe_msg_t m = {0};
        double val = 0.0;
        if (datahub_read_shadow(hub, g_tag_metadata[i].id, &m) == 0) val = m.payload.val.f;

        char buf[128];
        snprintf(buf, sizeof(buf), "%-8u | %-16.16s | %.2f", g_tag_metadata[i].id,
                 g_tag_metadata[i].name, val);
        hal->draw_text(hal, 4, 4 + (int)i, buf, UI_COLOR_DEFAULT);
    }
}

static void dashboard_on_draw(ui_page_t *page, ui_service_t *service) {
    (void)page;
    ui_hal_t *hal = service->hal;
    if (!hal) return;

    switch (g_ui_state.current_page) {
        case PAGE_DASHBOARD:
            render_dashboard(hal, service->hub);
            break;
        case PAGE_IO_DETAILS:
            render_io_details(hal, service->hub);
            break;
        case PAGE_ALARM:
            draw_common_frame(hal, "ALARMS");
            hal->draw_text(hal, 15, 7, "NO ACTIVE ALARMS", UI_COLOR_GREEN);
            break;
        default:
            break;
    }
}

static void dashboard_on_input(ui_page_t *page, ui_service_t *service, int key) {
    (void)page;
    switch (key) {
        case KEY_F(1):
            g_ui_state.current_page = PAGE_DASHBOARD;
            break;
        case KEY_F(2):
            g_ui_state.current_page = PAGE_IO_DETAILS;
            break;
        case KEY_F(3):
            g_ui_state.current_page = PAGE_ALARM;
            break;
        case 'q':
        case 'Q':
            ui_service_stop(service);
            break;
    }
}

static ui_page_t g_dashboard_page = {
    .name = "Main",
    .on_draw = dashboard_on_draw,
    .on_input = dashboard_on_input,
};

void ui_product_init(ui_service_t *service) {
    ui_service_push_page(service, &g_dashboard_page);
}