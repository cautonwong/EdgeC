#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif
#include <locale.h>
#include <ncursesw/ncurses.h>  // 显式引用宽字符版头文件
#include <stdlib.h>

#include "edge_ui.h"

typedef struct {
    WINDOW *win;
} ncurses_priv_t;

static void _ncurses_init(ui_hal_t *hal) {
    setlocale(LC_ALL, "");  // 驱动层也强制设置
    ncurses_priv_t *priv = (ncurses_priv_t *)hal->priv;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(10);
    start_color();
    init_pair(UI_COLOR_RED, COLOR_RED, COLOR_BLACK);
    init_pair(UI_COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(UI_COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(UI_COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(UI_COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
    priv->win = stdscr;
}

static void _ncurses_clear(ui_hal_t *hal) {
    (void)hal;
    erase();
}

static void _ncurses_draw_text(ui_hal_t *hal, int y, int x, const char *str, int color) {
    (void)hal;
    if (color > 0) attron(COLOR_PAIR(color));
    // 使用 mvaddstr，它在 ncursesw 中会自动处理 UTF-8
    mvaddstr(y, x, str);
    if (color > 0) attroff(COLOR_PAIR(color));
}

static void _ncurses_draw_pixel(ui_hal_t *hal, int y, int x, int color) {
    (void)hal;
    (void)color;
    mvaddch(y, x, '*');
}

static void _ncurses_draw_box(ui_hal_t *hal, int y, int x, int h, int w) {
    (void)hal;
    attron(A_BOLD);
    mvhline(y, x, 0, w);
    mvhline(y + h, x, 0, w);
    mvvline(y, x, 0, h);
    mvvline(y, x + w, 0, h);
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + w, ACS_URCORNER);
    mvaddch(y + h, x, ACS_LLCORNER);
    mvaddch(y + h, x + w, ACS_LRCORNER);
    attroff(A_BOLD);
}

static int _ncurses_get_key(ui_hal_t *hal) {
    (void)hal;
    return getch();
}

static void _ncurses_refresh(ui_hal_t *hal) {
    (void)hal;
    refresh();
}

static void _ncurses_destroy(ui_hal_t *hal) {
    (void)hal;
    endwin();
}

ui_hal_t *ui_hal_ncurses_create(void) {
    ui_hal_t *hal = calloc(1, sizeof(ui_hal_t));
    hal->name = "ncurses_w_simulator";
    hal->hal_init = _ncurses_init;
    hal->hal_clear = _ncurses_clear;
    hal->hal_draw_text = _ncurses_draw_text;
    hal->hal_draw_pixel = _ncurses_draw_pixel;
    hal->hal_draw_box = _ncurses_draw_box;
    hal->hal_get_key = _ncurses_get_key;
    hal->hal_refresh = _ncurses_refresh;
    hal->hal_destroy = _ncurses_destroy;
    hal->priv = calloc(1, sizeof(ncurses_priv_t));
    return hal;
}