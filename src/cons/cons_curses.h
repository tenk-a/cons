/**
 *  @file cons_curses.h
 *  @brief A console screen library implemented using ncurses or pdcurses.
 *  @author Masashi Kitamura ( https://github.com/tenk-a/ )
 *  @date   2024-12
 *  @license Boost Software License - Version 1.0
 */
#ifndef CONS_CURSES_H__
#define CONS_CURSES_H__

#define CONS_CURSES

// ncurses, pdcurses.
#define CONS_KEY_NONE           0
#define CONS_KEY_ERR            0xffff
#define CONS_KEY_DOWN           0x102
#define CONS_KEY_UP             0x103
#define CONS_KEY_LEFT           0x104
#define CONS_KEY_RIGHT          0x105
#define CONS_KEY_RETURN         0x0a
#define CONS_KEY_ESC            0x1B
#define CONS_KEY_SPACE          0x20

#define CONS_COL_DEFAULT        0
#define CONS_COL_BLACK          16
#define CONS_COL_BLUE           1
#define CONS_COL_RED            2
#define CONS_COL_MAGENTA        3
#define CONS_COL_GREEN          4
#define CONS_COL_CYAN           5
#define CONS_COL_YELLOW         6
#define CONS_COL_WHITE          7

#define CONS_COL_GRAY           8
#define CONS_COL_L_BLUE         9
#define CONS_COL_L_RED          10
#define CONS_COL_L_MAGENTA      11
#define CONS_COL_L_GREEN        12
#define CONS_COL_L_CYAN         13
#define CONS_COL_L_YELLOW       14
#define CONS_COL_L_WHITE        15

#define CONS_COL_LIGHT          8
#define CONS_COL_BACK_LIGHT     8
#define CONS_COL_REVERSE        0x10

#ifndef CONS_PRINTF_BUF_SIZE
#if defined __DOS__
#define CONS_PRINTF_BUF_SIZE    128
#else
#define CONS_PRINTF_BUF_SIZE    1024
#endif
#endif

#define CONS_CLOCK_PER_SEC      1000U
#define CONS_CLOCK_TO_MSEC(tm)  (tm) //((tm) * 1000 / CONS_CLOCK_PER_SEC)
#define CONS_MSEC_TO_CLOCK(ms)  (ms) //((ms)*CONS_CLOCK_PER_SEC / 1000)
#define CONS_TICK_PER_SEC       60U
#define CONS_TICK_TO_MSEC(tm)   ((tm) * 1000 / CONS_TICK_PER_SEC)
#define CONS_MSEC_TO_TICK(ms)   ((ms) * CONS_TICK_PER_SEC / 1000)

#if !defined(__DOS__)
typedef unsigned long long cons_clock_t;
typedef short          cons_pos_t;
#else
typedef unsigned long  cons_clock_t;
typedef signed char    cons_pos_t;
#endif
typedef unsigned char  cons_col_t;
typedef unsigned short cons_key_t;

int  cons_init(unsigned flags);
void cons_term(void);

void cons_updateBegin(void);
void cons_updateEnd(void);

cons_clock_t cons_clock(void);
cons_clock_t cons_tick(void);
cons_key_t   cons_key(void);
cons_pos_t   cons_screenWidth(void);
cons_pos_t   cons_screenHeight(void);

void cons_clear(void);

void cons_setxy(cons_pos_t x, cons_pos_t y);
void cons_setcolor(cons_col_t col);
void cons_resetcolor(cons_col_t col);

void cons_puts(char const* msg);
void cons_xyputs(cons_pos_t x, cons_pos_t y, char const* msg);
void cons_xycputs(cons_pos_t x, cons_pos_t y, cons_col_t col, char const* msg);

void cons_printf(char const* fmt, ...);
void cons_xyprintf(cons_pos_t x, cons_pos_t y, char const* fmt, ...);
void cons_xycprintf(cons_pos_t x, cons_pos_t y, cons_col_t col, char const* fmt, ...);

#define cons_setRefreshRect(n,x,y,w,h)

#endif //CONS_CURSES_H__
