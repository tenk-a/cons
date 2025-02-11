/**
 *  @file cons_pc98.h
 *  @brief A console screen library for pc98 msdos.
 *  @author Masashi Kitamura ( https://github.com/tenk-a/ )
 *  @date   2024-12
 *  @license Boost Software License - Version 1.0
 */
#ifndef CONS_P98_H__
#define CONS_P98_H__

#define CONS_PC98

#define CONS_KEY_NONE           0
#define CONS_KEY_DOWN           0x3D00
#define CONS_KEY_UP             0x3A00
#define CONS_KEY_LEFT           0x3B00
#define CONS_KEY_RIGHT          0x3C00
#define CONS_KEY_RETURN         0x1C0D
#define CONS_KEY_ESC            0x1B
#define CONS_KEY_SPACE          0x20
#define CONS_KEY_ERR            0xFFFF

#define CONS_COL_DEFAULT        7
#define CONS_COL_BLACK          0
#define CONS_COL_BLUE           1
#define CONS_COL_RED            2
#define CONS_COL_MAGENTA        3
#define CONS_COL_GREEN          4
#define CONS_COL_CYAN           5
#define CONS_COL_YELLOW         6
#define CONS_COL_WHITE          7
#if 0
#define CONS_COL_GRAY           8
#define CONS_COL_L_BLUE         9
#define CONS_COL_L_RED          10
#define CONS_COL_L_MAGENTA      11
#define CONS_COL_L_GREEN        12
#define CONS_COL_L_CYAN         13
#define CONS_COL_L_YELLOW       14
#define CONS_COL_L_WHITE        15
#endif

#define CONS_COL_LIGHT          0
#define CONS_COL_BACK_LIGHT     0
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
#define CONS_MSEC_TO_CLOCK(ms)  (ms) //((ms) * CONS_CLOCK_PER_SEC / 1000)
#define CONS_TICK_PER_SEC       60U
#define CONS_TICK_TO_MSEC(tm)   ((tm) * 1000 / CONS_TICK_PER_SEC)
#define CONS_MSEC_TO_TICK(ms)   ((ms) * CONS_TICK_PER_SEC / 1000)

typedef unsigned long  cons_clock_t;
typedef signed char    cons_pos_t;
typedef unsigned char  cons_col_t;
typedef unsigned short cons_key_t;

int  cons_init(unsigned flags);
void cons_term(void);

void cons_updateBegin(void);
void cons_updateEnd(void);

void cons_clear(void);
void cons_puts(char const* msg);
void cons_xyputs(cons_pos_t x, cons_pos_t y, char const* msg);
void cons_xycputs(cons_pos_t x, cons_pos_t y, cons_col_t col, char const* msg);
void cons_setcolor(cons_col_t co);

#if 1 // private name.
    extern cons_clock_t _cons_PRIVATE_clock;
    extern cons_clock_t _cons_PRIVATE_tick;
    extern cons_key_t   _cons_PRIVATE_key;
    extern cons_pos_t   _cons_PRIVATE_screen_width;
    extern cons_pos_t   _cons_PRIVATE_screen_height;
    extern cons_pos_t   _cons_PRIVATE_cur_x;
    extern cons_pos_t   _cons_PRIVATE_cur_y;
    char* _cons_PRIVATE_sprintf(char const* fmt, ...);
#endif

#define CONS_COLOR_DEFAULT  7
#define cons_clock()        (_cons_PRIVATE_clock)
#define cons_tick()         (_cons_PRIVATE_tick)
#define cons_key()          (_cons_PRIVATE_key)
#define cons_screenWidth()  (_cons_PRIVATE_screen_width)
#define cons_screenHeight() (_cons_PRIVATE_screen_height)
#define cons_setxy(x,y)     (_cons_PRIVATE_cur_x=(x), _cons_PRIVATE_cur_y=(y))
#define cons_resetcolor(c)  cons_setcolor(CONS_COLOR_DEFAULT)

#define cons_printf(...)    cons_puts(_cons_PRIVATE_sprintf(__VA_ARGS__))
#define cons_xyprintf(x,y, ...) \
        cons_xyputs((x),(y), _cons_PRIVATE_sprintf(__VA_ARGS__))
#define cons_xycprintf(x,y,c, ...)  \
        cons_xycputs((x),(y),(c), _cons_PRIVATE_sprintf(__VA_ARGS__))

#define CONS_REFRESH_RECT_N     4
void cons_setRefreshRect(unsigned char n, cons_pos_t x, cons_pos_t y, cons_pos_t w, cons_pos_t h);

#endif //CONS_P98_H__
