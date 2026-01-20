#include "ui_driver_ncurses.h"
#include <ncurses.h>

void ui_driver_ncurses_init(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100); // Non-blocking getch
}

void ui_driver_ncurses_teardown(void) {
    endwin();
}

void ui_driver_ncurses_draw_text(int y, int x, const char* text) {
    mvprintw(y, x, "%s", text);
}
