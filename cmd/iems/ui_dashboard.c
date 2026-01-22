#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "contracts.h"
#include "edge_log.h"
#include "edge_ui.h"

#define MAX_CATEGORIES 9  // 限制为 1-9 个数字键
#define MAX_TAGS_PER_CATEGORY 32

typedef struct {
    const char *name;
    uint32_t tag_indices[MAX_TAGS_PER_CATEGORY];
    int tag_count;
} ui_category_t;

static ui_category_t g_categories[MAX_CATEGORIES];
static int g_category_count = 0;
static int g_current_category_idx = 0;

static int get_or_create_category(const char *name) {
    for (int i = 0; i < g_category_count; i++) {
        if (strcmp(g_categories[i].name, name) == 0) return i;
    }
    if (g_category_count < MAX_CATEGORIES) {
        g_categories[g_category_count].name = name;
        g_categories[g_category_count].tag_count = 0;
        return g_category_count++;
    }
    return -1;
}

static void _ui_discover_categories() {
    g_category_count = 0;
    for (uint32_t i = 0; i < g_tag_metadata_len; i++) {
        const edge_tag_meta_t *meta = &g_tag_metadata[i];
        const char *category_name = meta->category ? meta->category : "General";
        int cat_idx = get_or_create_category(category_name);
        if (cat_idx != -1) {
            ui_category_t *cat = &g_categories[cat_idx];
            if (cat->tag_count < MAX_TAGS_PER_CATEGORY) {
                cat->tag_indices[cat->tag_count++] = i;
            }
        }
    }
}

static void draw_frame(ui_hal_t *hal, const char *title) {
    hal->hal_clear(hal);
    hal->hal_draw_box(hal, 0, 0, 20, 70);

    char buf[80];
    snprintf(buf, sizeof(buf), "--- IEMS Dashboard: %s ---", title);
    hal->hal_draw_text(hal, 0, 2, buf, UI_COLOR_CYAN);

    // [整改] 使用数字键作为页签提示
    int x_offset = 2;
    for (int i = 0; i < g_category_count; i++) {
        snprintf(buf, sizeof(buf), "[%d]%s ", i + 1, g_categories[i].name);
        hal->hal_draw_text(hal, 19, x_offset, buf,
                           (i == g_current_category_idx) ? UI_COLOR_YELLOW : UI_COLOR_BLUE);

        size_t len_bytes = strlen(buf);
        wchar_t wbuf[len_bytes + 1];
        mbstowcs(wbuf, buf, len_bytes + 1);
        int width = wcswidth(wbuf, len_bytes);
        x_offset += (width > 0) ? width : (int)len_bytes;
    }
    hal->hal_draw_text(hal, 19, 60, "[Q]uit", UI_COLOR_RED);
}

static void render_category_page(ui_hal_t *hal, datahub_t *hub) {
    if (g_category_count == 0) {
        draw_frame(hal, "No Data");
        return;
    }

    ui_category_t *current_cat = &g_categories[g_current_category_idx];
    draw_frame(hal, current_cat->name);

    int y_pos = 2;
    for (int i = 0; i < current_cat->tag_count && y_pos < 18; i++) {
        uint32_t meta_idx = current_cat->tag_indices[i];
        const edge_tag_meta_t *meta = &g_tag_metadata[meta_idx];

        pipe_msg_t msg = {0};
        double val = 0.0;
        if (datahub_read_shadow(hub, meta->id, &msg) == 0) {
            val = msg.payload.val.f;
        }

        char line[128];
        snprintf(line, sizeof(line), "%-25.25s : %10.2f %-10.10s", meta->name, val,
                 meta->unit ? meta->unit : "");
        hal->hal_draw_text(hal, y_pos++, 2, line, (val > 0) ? UI_COLOR_GREEN : UI_COLOR_DEFAULT);
    }
    hal->hal_refresh(hal);
}

static void dashboard_on_draw(ui_page_t *page, ui_service_t *service) {
    (void)page;
    if (service->hal) render_category_page(service->hal, service->hub);
}

static void dashboard_on_input(ui_page_t *page, ui_service_t *service, int key) {
    (void)page;
    // [整改] 改为数字键 1-9 切换
    if (key >= '1' && key <= ('0' + g_category_count)) {
        g_current_category_idx = key - '1';
    } else if (key == 'q' || key == 'Q') {
        ui_service_stop(service);
    }
}

static ui_page_t g_dashboard_page = {
    .name = "Main",
    .on_draw = dashboard_on_draw,
    .on_input = dashboard_on_input,
};

void ui_product_init(ui_service_t *service) {
    _ui_discover_categories();
    service->hal = ui_hal_ncurses_create();
    ui_service_push_page(service, &g_dashboard_page);
}
