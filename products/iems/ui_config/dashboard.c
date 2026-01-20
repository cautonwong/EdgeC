#include "edge_ui.h"
#include "edge_log.h"
#include "ui_driver_ncurses.h"
#include <time.h>

static void dashboard_on_draw(ui_page_t *page, ui_service_t *service) {
    (void)page;
    (void)service;

    // Clear screen
    erase();

    // Title
    ui_driver_ncurses_draw_text(1, 2, "IEMS Product Dashboard");
    
    // System Status
    ui_driver_ncurses_draw_text(3, 2, "System Status:  Running");
    
    // Data from DataHub (placeholders)
    ui_driver_ncurses_draw_text(5, 2, "--- Live Data ---");
    
    // In a real implementation, we would call datahub_read_shadow() here
    // using Tag IDs from the contract header to get live values.
    pipe_msg_t msg;
    double val1 = 0.0, val2 = 0.0;
    
    // Example: Read 'voltage' (assuming tag_id 1)
    if (datahub_read_shadow(service->hub, 1, &msg) == 0) {
        val1 = msg.payload.val.f;
    }
    // Example: Read 'current' (assuming tag_id 2)
    if (datahub_read_shadow(service->hub, 2, &msg) == 0) {
        val2 = msg.payload.val.f;
    }
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "  Voltage (V):    %.2f", val1);
    ui_driver_ncurses_draw_text(6, 2, buffer);
    
    snprintf(buffer, sizeof(buffer), "  Current (A):    %.2f", val2);
    ui_driver_ncurses_draw_text(7, 2, buffer);
    
    // Timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);
    ui_driver_ncurses_draw_text(10, 2, buffer);

    // Refresh the screen
    refresh();
}

// Define the dashboard page
static ui_page_t g_dashboard_page = {
    .name = "Dashboard",
    .on_draw = dashboard_on_draw,
};

// Implement the product-specific initializer
void ui_product_init(ui_service_t *service) {
    LOG_INFO(LOG_CAT_UI, "IEMS product UI initializing...");
    // Push the dashboard as the initial page
    ui_service_push_page(service, &g_dashboard_page);
}
