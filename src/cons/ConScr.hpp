/**
 *  @file ConScr.hpp
 *  @brief cons c++ wrapper
 *  @date 2023-12 - 2025
 *  @license Boost Software License - Version 1.0
 */
#ifndef CONSCR_HPP_INCLUDED
#define CONSCR_HPP_INCLUDED

#include "cons.h"

class ConScr {
public:
    enum { black=0, blue, red, magenta, green, cyan, yellow, white };

    enum {
        KEY_NONE   = CONS_KEY_NONE,
        KEY_DOWN   = CONS_KEY_DOWN,
        KEY_UP     = CONS_KEY_UP,
        KEY_LEFT   = CONS_KEY_LEFT,
        KEY_RIGHT  = CONS_KEY_RIGHT,
        KEY_RETURN = CONS_KEY_RETURN,
        KEY_ESC    = CONS_KEY_ESC,
        KEY_SPACE  = CONS_KEY_SPACE,
    };

    static bool init() { return cons_init(0) != 0; }
    static void term() { return cons_term(); }

    static void updateBegin() { cons_updateBegin(); }
    static void updateEnd() { cons_updateEnd(); }

    static void clear() { cons_clear(); }

    static int width()  { return (int)cons_screenWidth();  }
    static int height() { return (int)cons_screenHeight(); }

    static bool hasColor() { return cons_hascolor(); }
    static void setColor(int co) { cons_setcolor(convColNo(co)); }

    static void put(char const* s) { cons_puts(s); }
    static void setXY(short x, short y) { cons_setxy(x, y); }
    static void setYX(short y, short x) { setXY(x, y); }
    static void xyPut(short x, short y, char const* s) { cons_xyputs(x, y, s); }
    static void yxPut(short y, short x, char const* s) { xyPut(x, y, s); }

    static int  getCh() { return (int)cons_key(); }
    static bool kbHit() { return cons_kbhit(); }

private:
    static unsigned convColNo(unsigned c) {
     #if    CONS_COL_BLACK  == 0 && CONS_COL_BLUE    == 1   \
         && CONS_COL_RED    == 2 && CONS_COL_MAGENTA == 3   \
         && CONS_COL_GREEN  == 4 && CONS_COL_CYAN    == 5   \
         && CONS_COL_YELLOW == 6 && CONS_COL_WHITE   == 7
        return c;
     #else
        static unsigned short const s_tbl[] = {
            CONS_COL_BLACK , CONS_COL_BLUE,
            CONS_COL_RED   , CONS_COL_MAGENTA,
            CONS_COL_GREEN , CONS_COL_CYAN,
            CONS_COL_YELLOW, CONS_COL_WHITE,
        };
        return s_tbl[c & 7];
     #endif
    }
};

#endif
