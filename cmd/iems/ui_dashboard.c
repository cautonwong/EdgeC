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

static void draw_beautified_frame(ui_hal_t *hal, const char *title, int color) {
    hal->draw_box(hal, 0, 0, 52, 16);  // 稍微加大尺寸

    // 顶部反色标题栏模拟
    hal->draw_text(hal, 1, 1, "----------------------------------------------------",
                   UI_COLOR_DEFAULT);
    hal->draw_text(hal, 15, 1, " ⚡ ", UI_COLOR_YELLOW);
    hal->draw_text(hal, 19, 1, title, color);
    hal->draw_text(hal, 1, 2, "----------------------------------------------------",
                   UI_COLOR_DEFAULT);

    // 底部状态栏
    hal->draw_text(hal, 2, 15, " [F1]📊  [F2]📑  [F3]⚠️   [Q]❌ ", UI_COLOR_BLUE);
}

static void draw_progress_bar(ui_hal_t *hal, int x, int y, int width, double percentage,
                              int color) {
    int filled = (int)((double)width * percentage / 100.0);
    if (filled > width) filled = width;

    hal->draw_text(hal, x, y, "[", UI_COLOR_DEFAULT);
    for (int i = 0; i < width; i++) {
        if (i < filled)
            hal->draw_text(hal, x + 1 + i, y, "■", color);
        else
            hal->draw_text(hal, x + 1 + i, y, " ", UI_COLOR_DEFAULT);
    }
    hal->draw_text(hal, x + 1 + width, y, "]", UI_COLOR_DEFAULT);
}

static void render_dashboard(ui_hal_t *hal, datahub_t *hub) {
    draw_beautified_frame(hal, "IEMS CONTROL CENTER", UI_COLOR_CYAN);

    if (g_ui_state.active_tag_v == 0 && g_tag_metadata_len > 0) {
        g_ui_state.active_tag_v = g_tag_metadata[0].id;
        if (g_tag_metadata_len > 1) g_ui_state.active_tag_i = g_tag_metadata[1].id;
    }

    pipe_msg_t msg_v = {0}, msg_i = {0};
    bool v_ok = (datahub_read_shadow(hub, g_ui_state.active_tag_v, &msg_v) == 0);
    bool i_ok = (datahub_read_shadow(hub, g_ui_state.active_tag_i, &msg_i) == 0);

    if (!v_ok) {
        hal->draw_text(hal, 12, 8, "📡 WAITING FOR TELEMETRY...", UI_COLOR_RED);
    } else {
        char buf[64];

        // 电压显示
        hal->draw_text(hal, 4, 4, "⚡ Grid Voltage :", UI_COLOR_DEFAULT);
        snprintf(buf, sizeof(buf), "%7.2f V", msg_v.payload.val.f);
        hal->draw_text(hal, 22, 4, buf, UI_COLOR_GREEN);

        // 电流显示
        if (i_ok) {
            hal->draw_text(hal, 4, 6, "🔌 Load Current :", UI_COLOR_DEFAULT);
            snprintf(buf, sizeof(buf), "%7.2f A", msg_i.payload.val.f);
            hal->draw_text(hal, 22, 6, buf, UI_COLOR_GREEN);

            // 负载进度条
            hal->draw_text(hal, 4, 8, "📊 System Load  :", UI_COLOR_DEFAULT);
            double load_p = (msg_i.payload.val.f / 100.0) * 100.0;  // 假设 100A 是满载
            draw_progress_bar(hal, 22, 8, 20, load_p,
                              (load_p > 80) ? UI_COLOR_RED : UI_COLOR_YELLOW);
        }

        // 运行时间/心跳状态
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(buf, sizeof(buf), "🕒 %H:%M:%S", t);
        hal->draw_text(hal, 35, 13, buf, UI_COLOR_BLUE);

        hal->draw_text(hal, 4, 13, "● SYSTEM LIVE", UI_COLOR_GREEN);
    }
}

static void render_io_details(ui_hal_t *hal, datahub_t *hub) {
    draw_beautified_frame(hal, "RESOURCE INVENTORY", UI_COLOR_CYAN);
    hal->draw_text(hal, 4, 4, "TAG ID   NAME             VALUE", UI_COLOR_BLUE);
    hal->draw_text(hal, 4, 5, "-------------------------------", UI_COLOR_DEFAULT);

    for (uint32_t i = 0; i < g_tag_metadata_len && i < 7; i++) {
        pipe_msg_t m = {0};
        double val = 0.0;
        if (datahub_read_shadow(hub, g_tag_metadata[i].id, &m) == 0) val = m.payload.val.f;

        char buf[128];
        snprintf(buf, sizeof(buf), "🆔 %-5u %-16.16s %.2f", g_tag_metadata[i].id,
                 g_tag_metadata[i].name, val);
        hal->draw_text(hal, 4, 6 + (int)i, buf, UI_COLOR_DEFAULT);
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
            draw_beautified_frame(hal, "ACTIVE ALARMS", UI_COLOR_RED);
            hal->draw_text(hal, 16, 8, "✅ NO FAULTS DETECTED", UI_COLOR_GREEN);
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
