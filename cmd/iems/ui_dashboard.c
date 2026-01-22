#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>

#include "edge_log.h"
#include "edge_ui.h"
#include "contracts.h"

// Define limits for our dynamic structures
#define MAX_CATEGORIES 16
#define MAX_TAGS_PER_CATEGORY 32

// New data structures for dynamic UI
typedef struct {
    const char* name;
    uint32_t tag_indices[MAX_TAGS_PER_CATEGORY];
    int tag_count;
} ui_category_t;

static ui_category_t g_categories[MAX_CATEGORIES];
static int g_category_count = 0;
static int g_current_category_idx = 0;

// Helper to find or create a category
static int get_or_create_category(const char *name) {
    for (int i = 0; i < g_category_count; i++) {
        if (strcmp(g_categories[i].name, name) == 0) {
            return i;
        }
    }
    if (g_category_count < MAX_CATEGORIES) {
        g_categories[g_category_count].name = name;
        g_categories[g_category_count].tag_count = 0;
        return g_category_count++;
    }
    return -1; // No space for new categories
}

// New function to discover categories from metadata at startup
static void _ui_discover_categories() {
    for (uint32_t i = 0; i < g_tag_metadata_len; i++) {
        const edge_tag_meta_t *meta = &g_tag_metadata[i];
        const char* category_name = meta->category ? meta->category : "Uncategorized";
        
        int cat_idx = get_or_create_category(category_name);
        
        if (cat_idx != -1) {
            ui_category_t *cat = &g_categories[cat_idx];
            if (cat->tag_count < MAX_TAGS_PER_CATEGORY) {
                cat->tag_indices[cat->tag_count++] = i;
            }
        }
    }
}

// --- Rewritten UI Logic ---

static void draw_frame(ui_hal_t *hal, const char *title) {
    hal->hal_clear(hal); // Clear the screen first
    hal->hal_draw_box(hal, 0, 0, 70, 20); // Make box bigger
    char buf[80];
    snprintf(buf, sizeof(buf), "--- IEMS Dashboard: %s ---", title);
    hal->hal_draw_text(hal, 0, 2, buf, UI_COLOR_CYAN);
    
    // Dynamic footer showing categories
    int x_offset = 2;
    for (int i = 0; i < g_category_count; i++) {
        snprintf(buf, sizeof(buf), "[F%d]%s ", i + 1, g_categories[i].name);
        hal->hal_draw_text(hal, 20, x_offset, buf, (i == g_current_category_idx) ? UI_COLOR_YELLOW : UI_COLOR_BLUE);
        
        // Correctly calculate screen width for multi-byte characters
        size_t len_bytes = strlen(buf);
        wchar_t wbuf[len_bytes + 1];
        mbstowcs(wbuf, buf, len_bytes + 1);
        int width = wcswidth(wbuf, len_bytes);
        if (width > 0) {
            x_offset += width;
        } else {
            x_offset += (int)len_bytes; // Fallback
        }
    }
    hal->hal_draw_text(hal, 20, x_offset, "[Q]uit", UI_COLOR_BLUE);
    hal->hal_refresh(hal);
}

static void render_category_page(ui_hal_t *hal, datahub_t *hub) {
    if (g_category_count == 0) {
        draw_frame(hal, "No Categories Found");
        return;
    }

    ui_category_t *current_cat = &g_categories[g_current_category_idx];
    draw_frame(hal, current_cat->name);

    int y_pos = 2;
    for (int i = 0; i < current_cat->tag_count; i++) {
        uint32_t meta_idx = current_cat->tag_indices[i];
        const edge_tag_meta_t *meta = &g_tag_metadata[meta_idx];
        
        pipe_msg_t msg = {0};
        double val = 0.0;
        if (datahub_read_shadow(hub, meta->id, &msg) == 0) {
            val = msg.payload.val.f;
        }

        char line[128];
        snprintf(line, sizeof(line), "%-25.25s : %10.2f %-10.10s", 
                 meta->name, 
                 val,
                 meta->unit ? meta->unit : "");
        hal->hal_draw_text(hal, y_pos++, 2, line, (val > 0) ? UI_COLOR_GREEN : UI_COLOR_DEFAULT);
    }
    hal->hal_refresh(hal);
}

static void dashboard_on_draw(ui_page_t *page, ui_service_t *service) {
    (void)page;
    ui_hal_t *hal = service->hal;
    if (!hal) return;
    render_category_page(hal, service->hub);
}

static void dashboard_on_input(ui_page_t *page, ui_service_t *service, int key) {
    (void)page;
    if (key >= KEY_F(1) && key <= KEY_F(MAX_CATEGORIES)) {
        int cat_idx = key - KEY_F(1);
        if (cat_idx < g_category_count) {
            g_current_category_idx = cat_idx;
        }
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
    // 1. Discover the UI structure from metadata
    _ui_discover_categories();

    // 2. Create and inject the ncurses driver
    ui_hal_t *hal = ui_hal_ncurses_create();
    service->hal = hal;

    // 3. Push the single, dynamic page
    ui_service_push_page(service, &g_dashboard_page);
}
