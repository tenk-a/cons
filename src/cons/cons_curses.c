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
 #define PDC_WIDE         1
 #define PDC_FORCE_UTF8   1
 #include <curses.h>
 #define CONS_USE_PDCURSES
#elif defined(__DOS__)
 #ifndef __FLAT__
  //#define CHTYPE_16     1
  #define CHTYPE_32       1
 #endif
 #include <curses.h>
 #define CONS_USE_PDCURSES
#else   // mac,linux,unix
 #include <sys/time.h>
 #include <locale.h>
 #include <ncurses.h>
#endif

unsigned short const _cons_PRIVATE_curses_key[8] = {
    KEY_CODE_YES ,  // 0x00
    KEY_BREAK    ,  // 0x01 + 0x100 -> 0x101  PDCursesMod-WIDE 0x01 + 0xec00 -> 0xec01
    KEY_DOWN     ,  // 0x02
    KEY_UP       ,  // 0x03
    KEY_LEFT     ,  // 0x04
    KEY_RIGHT    ,  // 0x05
    KEY_HOME     ,  // 0x06
    KEY_BACKSPACE,  // 0x07
};

static cons_pos_t   _cons_screen_width;
static cons_pos_t   _cons_screen_height;
static cons_clock_t _cons_start_clock;
static cons_clock_t _cons_cur_clock;
static cons_clock_t _cons_cur_tick;
static int          _cons_cur_key;
static char         _cons_has_color;
#if defined(_WIN32) && defined(CONS_USE_UNICODE)
static int          _cons_win_codepage;
#endif

static cons_clock_t _cons_getClock() {
 #if defined(_WIN32)
    static unsigned __int64 per_sec = 0;
    unsigned __int64        count   = 0;
    if (!per_sec)
        QueryPerformanceFrequency((LARGE_INTEGER*)&per_sec);
    QueryPerformanceCounter((LARGE_INTEGER*)&count);
    return count * CONS_CLOCK_PER_SEC / per_sec;
 #elif defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (cons_clock_t)ts.tv_sec * 1000000ULL + (ts.tv_nsec / 1000ULL);
 #elif defined(__DJGPP__)
    return (cons_clock_t)(uclock() * CONS_CLOCK_PER_SEC / UCLOCKS_PER_SEC);
 #elif defined(__DOS__)
    return (cons_clock_t)(clock() * CONS_CLOCK_PER_SEC / CLOCKS_PER_SEC);
 #else
    struct timeval tv = {0,0};
    gettimeofday(&tv, NULL);
    return (cons_clock_t)((tv.tv_sec * 1000000ULL + tv.tv_usec) * CONS_CLOCK_PER_SEC / 1000000ULL);
 #endif
}

// sleep.
static void _cons_clock_sleep(cons_clock_t count) {
 #if defined(_WIN32)
    Sleep(count * 1000 / CONS_CLOCK_PER_SEC);
 #elif defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 199309L) || defined(__APPLE__)
    struct timespec ts;
    ts.tv_sec  = count / CONS_CLOCK_PER_SEC;
    count %= CONS_CLOCK_PER_SEC;
    ts.tv_nsec = (long)(count * (1000000000LL / CONS_CLOCK_PER_SEC));
    nanosleep(&ts, &ts);
 #elif !defined(__DOS__)
    usleep(count * 1000000LL / CONS_CLOCK_PER_SEC);
 #else
    (void)count;
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
 #elif defined(_WIN32) && defined(CONS_USE_UNICODE)
    _cons_win_codepage = GetConsoleOutputCP();
    SetConsoleOutputCP(65001);
 #endif

    _cons_start_clock = _cons_getClock();

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);    // Non-blocking getch.
    curs_set(0);
    _cons_updateScreenSize();

    _cons_has_color = 1;
    if (has_colors() == FALSE) {
        _cons_has_color = 0;
        endwin();
        return 0;
    }

    {
        static int const cols[8] = {
            COLOR_BLACK, COLOR_BLUE, COLOR_RED, COLOR_MAGENTA,
            COLOR_GREEN, COLOR_CYAN, COLOR_YELLOW, COLOR_WHITE
        };
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
    return 1;
}

void cons_term(void) {
    endwin();
 #if (defined(_WIN32) && defined(CONS_USE_UNICODE))
    SetConsoleOutputCP(_cons_win_codepage);
 #endif
}

static inline cons_clock_t _cons_getClockN(void) {
    return _cons_getClock() - _cons_start_clock;
}

void cons_updateBegin(void) {
    _cons_cur_clock = _cons_getClockN();
    _cons_cur_tick  = _cons_cur_clock * CONS_TICK_PER_SEC / CONS_CLOCK_PER_SEC;
    _cons_cur_key   = (cons_key_t)getch();
    _cons_updateScreenSize();
}

void cons_updateEnd(void) {
    cons_clock_t now  = _cons_getClockN();
    cons_clock_t next = (_cons_cur_tick + 1) * CONS_CLOCK_PER_SEC / CONS_TICK_PER_SEC;
    cons_clock_t dif  = (next > now) ? next - now : 0;
    _cons_clock_sleep(dif);
    do {
        now = _cons_getClockN();
    } while (now < next);
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

int         cons_kbhit(void) {
    return _cons_cur_key != 0;
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

int  cons_hascolor(void) {
    return _cons_has_color;
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
