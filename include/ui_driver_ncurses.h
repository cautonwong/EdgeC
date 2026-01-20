#ifndef UI_DRIVER_NCURSES_H
#define UI_DRIVER_NCURSES_H

void ui_driver_ncurses_init(void);
void ui_driver_ncurses_teardown(void);
void ui_driver_ncurses_draw_text(int y, int x, const char* text);

#endif // UI_DRIVER_NCURSES_H
