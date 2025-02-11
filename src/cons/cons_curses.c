/**
 *  @file cons_curses.c
 *  @brief A console screen library implemented using ncurses or pdcurses.
 *  @author Masashi Kitamura ( https://github.com/tenk-a/ )
 *  @date   2024-12
 *  @license Boost Software License - Version 1.0
 */

#include "cons_curses.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#undef MOUSE_MOVED
#endif

#if defined(CONS_USE_PDCURSES)
#include <curses.h>
#else
#include <sys/time.h>
#include <ncurses.h>
#include <locale.h>
#endif

static cons_pos_t   _cons_screen_width;
static cons_pos_t   _cons_screen_height;
static cons_clock_t _cons_start_clock;
static cons_clock_t _cons_cur_clock;
static cons_clock_t _cons_cur_tick;
static int          _cons_cur_key;
#if defined(_WIN32) && defined(CONS_USE_UNICODE)
static int          _cons_win_codepage;
#endif

static cons_clock_t _con_getCurrentTimer() {
 #if defined __DJGPP__
    return (cons_clock_t)(uclock() * CONS_CLOCK_PER_SEC / UCLOCKS_PER_SEC);
 #elif defined(__DOS__) || defined(_WIN32)
    return (cons_clock_t)(clock() * CONS_CLOCK_PER_SEC / CLOCKS_PER_SEC);
 #else
    struct timeval tv = {0,0};
    gettimeofday(&tv, NULL);
    return (cons_clock_t)CONS_MSEC_TO_CLOCK(tv.tv_sec * 1000U + (tv.tv_usec / 1000U));
 #endif
}

static void _cons_updateScreenSize(void) {
    int w = 80, h = 24;
    getmaxyx(stdscr, h, w);
    if (sizeof(cons_pos_t) == 1) {
        if (w > 126) w = 126;
        if (h > 126) h = 126;
    }
    _cons_screen_width  = w;
    _cons_screen_height = h;
}

int cons_init(unsigned flags) {
    (void)flags;
 #if !defined(CONS_USE_PDCURSES)
    setlocale(LC_ALL, "");
 #elif (defined(_WIN32) && defined(CONS_USE_UNICODE))   // nennotame
    _cons_win_codepage = GetConsoleOutputCP();
    SetConsoleOutputCP(65001);
 #endif

    _cons_start_clock = _con_getCurrentTimer();

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    _cons_updateScreenSize();

    if (has_colors() == FALSE) {
        endwin();
        return 0;
    }

    start_color();
    {
        static int const cols[8] = { COLOR_BLACK, COLOR_BLUE, COLOR_RED, COLOR_MAGENTA,
                        COLOR_GREEN, COLOR_CYAN, COLOR_YELLOW, COLOR_WHITE };
        int i = 0;
        start_color();
        for (i = 0; i < 8; ++i) {
            if (i)
                init_pair(   i,   cols[i], COLOR_BLACK);
            init_pair(     8|i, 8|cols[i], COLOR_BLACK);
            init_pair(0x10|  i, COLOR_BLACK,   cols[i]);
            init_pair(0x10|8|i, COLOR_BLACK, 8|cols[i]);
        }
    }

    timeout(50);    // getch timeout (milliseconds)
    return 1;
}

void cons_term(void) {
    endwin();
 #if (defined(_WIN32) && defined(CONS_USE_UNICODE))
    SetConsoleOutputCP(_cons_win_codepage);
 #endif
}

void cons_updateBegin(void) {
    _cons_cur_clock = (cons_clock_t)(_con_getCurrentTimer()-_cons_start_clock);
    _cons_cur_tick  = _cons_cur_clock * CONS_TICK_PER_SEC / CONS_CLOCK_PER_SEC;
    _cons_cur_key   = (cons_key_t)getch();

    _cons_updateScreenSize();
}

void cons_updateEnd(void) {
    refresh();
}

void cons_clear(void) {
 #if defined(CONS_USE_PDCURSES)
    clear();
 #else
    erase();
 #endif
}

cons_clock_t cons_clock(void) {
    return _cons_cur_clock;
}

cons_clock_t cons_tick(void) {
    return _cons_cur_tick;
}

cons_key_t   cons_key(void) {
    return (cons_key_t)_cons_cur_key;
}

cons_pos_t   cons_screenWidth(void) {
    return _cons_screen_width;
}

cons_pos_t   cons_screenHeight(void) {
    return _cons_screen_height;
}

void cons_setxy(cons_pos_t x, cons_pos_t y) {
    move(y, x);
}

void cons_setcolor(cons_col_t col) {
    attron(COLOR_PAIR(col)); // | A_BOLD);
}

void cons_resetcolor(cons_col_t col) {
    attroff(COLOR_PAIR(col)); // | A_BOLD);
}

void cons_puts(char const* s) {
    addstr(s);
}

void cons_xyputs(cons_pos_t x, cons_pos_t y, char const* s) {
    move(y, x);
    cons_puts(s);
}

void cons_xycputs(cons_pos_t x, cons_pos_t y, cons_col_t c, char const* s) {
    move(y, x);
    cons_setcolor(c);
    cons_puts(s);
}

void cons_printf(char const* fmt, ...) {
    char buf[CONS_PRINTF_BUF_SIZE];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, CONS_PRINTF_BUF_SIZE-1, fmt, arg);
    buf[CONS_PRINTF_BUF_SIZE-1] = 0;
    cons_puts(buf);
    va_end(arg);
}

void cons_xyprintf(cons_pos_t x, cons_pos_t y, char const* fmt, ...) {
    char buf[CONS_PRINTF_BUF_SIZE];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, CONS_PRINTF_BUF_SIZE-1, fmt, arg);
    buf[CONS_PRINTF_BUF_SIZE-1] = 0;
    cons_xyputs(x, y, buf);
    va_end(arg);
}

void cons_xycprintf(cons_pos_t x, cons_pos_t y, cons_col_t c, char const* fmt, ...) {
    char buf[CONS_PRINTF_BUF_SIZE];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, CONS_PRINTF_BUF_SIZE-1, fmt, arg);
    buf[CONS_PRINTF_BUF_SIZE-1] = 0;
    cons_xycputs(x, y, c, buf);
    va_end(arg);
}
