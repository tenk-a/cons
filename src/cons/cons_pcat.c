/**
 *  @file cons_pcat.c
 *  @brief A console screen library for pc(at) msdos.
 *  @author Masashi Kitamura ( https://github.com/tenk-a/ )
 *  @date   2024-12
 *  @license Boost Software License - Version 1.0
 */
#include "cons_pcat.h"
#include <i86.h>
#include <dos.h>
#include <conio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#if __STDC_VERSION__ >= 199901L || __cplusplus >= 201103L
 #include <stdint.h>
 #if !defined(__cplusplus)
  #include <stdbool.h>
 #endif
#else
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned char   bool;
#endif

#if defined __FLAT__
#undef __far
#define __far
#define MK_FAR_PTR(a,b)     ((a << 4) + (b))
#define _fmalloc            malloc
#define _ffree              free
#define int86               int386
#else
#define MK_FAR_PTR(a,b)     (((uint32_t)(a) << 16) | (b))
#endif

cons_clock_t _cons_PRIVATE_clock;
cons_clock_t _cons_PRIVATE_tick;
cons_key_t   _cons_PRIVATE_key;
cons_pos_t   _cons_PRIVATE_screen_width;
cons_pos_t   _cons_PRIVATE_screen_height;
cons_pos_t   _cons_PRIVATE_cur_x;
cons_pos_t   _cons_PRIVATE_cur_y;
cons_col_t   _cons_PRIVATE_col;

typedef struct CursorInfo {
    uint8_t     startScanLine;
    uint8_t     endScanLine;
    uint8_t     cursorVisible;
} CursorInfo;
static CursorInfo s_cursorInfo;

static uint16_t __far* s_textVram = NULL;
static uint16_t __far* s_textBuf  = NULL;

static cons_clock_t s_start_clock;
static uint8_t  s_saveVideoMode;
static uint8_t  s_curVideoMode;
static int      s_textVramW;
static int      s_textVramH;
static int      s_textBufW = 80;
static int      s_textBufH = 25;
static char     s_cons_sprintf_buf[CONS_PRINTF_BUF_SIZE];

typedef struct cons_rect_t {
    cons_pos_t  x, y;
    cons_pos_t  w, h;
} cons_rect_t;
static cons_rect_t  s_refresh_rect[CONS_REFRESH_RECT_N];

/** Get video mode
 */
static uint8_t getVideoMode() {
    union REGS regs;
    regs.h.ah = 0x0F;
    int86(0x10, &regs, &regs);
    return regs.h.al;
}

/** Set video mode.
 */
static void setVideoMode(uint8_t mode) {
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = mode;
    int86(0x10, &regs, &regs);
}

/** Get cursor state.
 */
static void getCursorInfo(CursorInfo* cursorInfo) {
    union REGS regs;
    regs.h.ah = 0x03;
    regs.h.bh = 0x00;  // current page.
    int86(0x10, &regs, &regs);
    cursorInfo->startScanLine = regs.h.ch;
    cursorInfo->endScanLine   = regs.h.cl;
    cursorInfo->cursorVisible = (regs.h.ch == 0x20) ? 0 : 1;
}

/** Hide cursor.
 */
static void hideCursor() {
    union REGS regs;
    regs.h.ah = 0x01;
    regs.w.cx = 0x2000;
    int86(0x10, &regs, &regs);
}

/** Show cursor.
 */
static void showCursor(CursorInfo const* cursorInfo) {
    union REGS regs;
    regs.h.ah = 0x01;
    regs.h.ch = cursorInfo->startScanLine;
    regs.h.cl = cursorInfo->endScanLine;
    int86(0x10, &regs, &regs);
}

/** Get text-screen width.
 */
static int getScreenWidth() {
    union REGS regs;
    regs.h.ah = 0x0F;
    int86(0x10, &regs, &regs);
    return regs.h.ah;
}

/** Get text-screen height.
 */
static int getScreenHeight() {
    union REGS regs;
    regs.w.ax = 0x1130;
    int86(0x10, &regs, &regs);
    return regs.h.dl;
}

static void updateWidthHeight() {
    s_textVramW = getScreenWidth();
    s_textVramH = getScreenHeight();
    if (sizeof(cons_pos_t) == 1) {
        _cons_PRIVATE_screen_width  = (s_textVramW >= 127) ? 127 : s_textVramW;
        _cons_PRIVATE_screen_height = (s_textVramH >= 127) ? 127 : s_textVramH;
    } else {
        _cons_PRIVATE_screen_width  = s_textVramW;
        _cons_PRIVATE_screen_height = s_textVramH;
    }
}

static void setBlinkMode(bool sw) {
    union REGS regs;
    regs.w.ax = 0x1003;
    regs.w.bx = sw;
    int86(0x10, &regs, &regs);
}

/** Is a key pressed?
 */
static bool kbHit() {
    union REGS regs;
    regs.h.ah = 0x0B; // Check Standard Input Status
    intdos(&regs, &regs);
    return regs.h.al != 0;
}

/** One key input
 */
static uint16_t getCh() {
    union REGS regs;
    regs.h.ah = 0x00;
    int86(0x16, &regs, &regs);
    if (regs.h.al == 0 || regs.h.al == 0xE0) {
        return (0xE000 | regs.h.ah);
    }
    return regs.h.al;
}

#if 0 //!defined __FLAT__
static uint16_t __far* getTextVramAddr(void) {
    union  REGS  regs;
    struct SREGS sregs;
    regs.w.ax = 0xFE00;
    regs.w.di = (uint16_t)s_textVram;
    sregs.es  = (uint16_t)((uint32_t)s_textVram >> 16);
    int86x(0x10, &regs, &regs, &sregs);
    return (regs.x.cflag) ? NULL : (uint16_t __far*)MK_FAR_PTR(sregs.es, regs.w.di);
}
#endif


#if 0
/** Screen refresh.
 */
static void consRefresh() {
    updateWidthHeight();
    if (s_textVramW == s_textBufW && s_textVramH == s_textBufH) {
        _fmemcpy(s_textVram, s_textBuf, s_textVramW * s_textVramH * sizeof(uint16_t));
    } else {
        unsigned dofs = 0, sofs = 0, y;
        unsigned w = s_textBufW < s_textVramW ? s_textBufW : s_textVramW;
        for (y = 0; y < s_textBufH; ++y) {
            _fmemcpy(&s_textVram[dofs], &s_textBuf[sofs], w * sizeof(uint16_t));
            dofs += s_textVramW;
            sofs += s_textBufW;
        }
    }
}
#endif

/** Screen refresh.
 */
static void consRefresh(void) {
    cons_rect_t const* rt = &s_refresh_rect[0];
    updateWidthHeight();
    if (rt->w==s_textBufW && rt->h==s_textBufH && rt->x==0 && rt->y==0
        && s_textVramW == s_textBufW && s_textVramH == s_textBufH
    ) {
        _fmemcpy(s_textVram, s_textBuf, s_textBufW * s_textBufH * sizeof(uint16_t));
    } else {
        cons_rect_t const* rt_e = &s_refresh_rect[CONS_REFRESH_RECT_N];
        do {
            unsigned    n, bytes, dofs, sofs;
            cons_pos_t  x,y,w,h;
            x  = rt->x;
            w  = rt->w;
            if (x <= 0) {
                w += x;
                x = 0;
                if (w <= 0)
                    continue;
            } else if (x + w > s_textBufW) {
                w = s_textBufW - x;
                if (w <= 0)
                    continue;
            }
            y  = rt->y;
            h  = rt->h;
            if (y <= 0) {
                h += y;
                y = 0;
                if (h <= 0)
                    continue;
            } else if (y + h > s_textBufH) {
                h = s_textBufH - y;
                if (h <= 0)
                    continue;
            }
            bytes = w * sizeof(uint16_t);
            sofs  = y * s_textBufW  + x;
            dofs  = y * s_textVramW + x;
            for (n = 0; n < h; ++n) {
                _fmemcpy(&s_textVram[dofs], &s_textBuf[sofs], bytes);
                sofs += s_textBufW;
                dofs += s_textVramW;
            }
        } while (++rt < rt_e);
    }
}

/** vblank start wait.
 */
static void vsyncWait(void) {
    enum { vga_status_port = 0x03DA };
    while ((inp(vga_status_port) & 0x08) != 0) { }
    while ((inp(vga_status_port) & 0x08) == 0) { }
}

static cons_clock_t getCurrentTimer(void) {
 #if defined __DJGPP__
    return (cons_clock_t)(uclock() * CONS_CLOCK_PER_SEC / UCLOCKS_PER_SEC);
 #else
    return (cons_clock_t)(clock() * CONS_CLOCK_PER_SEC / CLOCKS_PER_SEC);
 #endif
}

// ================================================================

/** Initialize.
 */
int cons_init(unsigned flags) {
    s_saveVideoMode = getVideoMode();
    if (flags & 1)  // text 40x25
        s_curVideoMode  = 0x01;
    else            // text 80x25
        s_curVideoMode  = 0x03;
    s_textBufW = (s_curVideoMode <= 1) ? 40 : 80;
    setVideoMode(s_curVideoMode);

    getCursorInfo(&s_cursorInfo);
 #if 1 //defined __FLAT__
    s_textVram      = (uint16_t __far*)MK_FAR_PTR(0xB800,0x0000);
 #else
    s_dosv          = 1;
    s_textVram      = getTextVramAddr();
    if (s_textVram == NULL) {
        s_textVram  = (uint16_t __far*)MK_FAR_PTR(0xB800,0x0000);
        s_dosv      = 0;
    }
 #endif
    updateWidthHeight();
    if (!s_textBuf) {
        size_t bytes = s_textBufW * s_textBufH * sizeof(uint16_t);
        s_textBuf    = (uint16_t __far*)_fmalloc(bytes);
    }
    _cons_PRIVATE_col   = CONS_COL_DEFAULT; // white
    setBlinkMode(0);
    hideCursor();
    cons_setRefreshRect(0, 0,0, s_textBufW, s_textBufH);
    cons_clear();
    consRefresh();
    s_start_clock = getCurrentTimer();

    return 1;
}

/** Terminate.
 */
void cons_term(void) {
    cons_setRefreshRect(0, 0,0, s_textBufW, s_textBufH);
    cons_clear();
    consRefresh();
    if (s_cursorInfo.cursorVisible)
        showCursor(&s_cursorInfo);
    _ffree(s_textBuf);
    s_textBuf = NULL;
    setVideoMode(s_saveVideoMode);
}

/**
 */
void cons_updateBegin(void) {
    cons_key_t   k;
    _cons_PRIVATE_clock  = getCurrentTimer() - s_start_clock;
    _cons_PRIVATE_tick   = _cons_PRIVATE_clock * 60 / CONS_CLOCK_PER_SEC;
    if (kbHit()) {
        do {
            k = getCh();
        } while (kbHit());
    } else {
        k = CONS_KEY_ERR;
    }
    _cons_PRIVATE_key = k;
    cons_setRefreshRect(0, 0,0, s_textBufW, s_textBufH);
}

/**
 */
void cons_updateEnd(void) {
    vsyncWait();
    consRefresh();
}

/** Set screen refresh rect.
 */
void cons_setRefreshRect(uint8_t no, cons_pos_t x, cons_pos_t y, cons_pos_t w, cons_pos_t h) {
    cons_rect_t* rt = &s_refresh_rect[no];
    assert(no < CONS_REFRESH_RECT_N);
    rt->x = x;
    rt->y = y;
    rt->w = w;
    rt->h = h;
}

/** Screen clear.
 */
void cons_clear(void) {
    unsigned x, y;
    uint16_t offset = 0;
    uint16_t co     = 7; //_cons_PRIVATE_col;

    updateWidthHeight();
    for (y = 0; y < s_textBufH; ++y) {
        for (x = 0; x < s_textBufW; ++x) {
            s_textBuf[offset++] = (co << 8) | ' ';
        }
    }
    cons_setxy(0, 0);
}

/** Set color
 */
void cons_setcolor(uint8_t co) {
    static uint8_t const tbl[] = { 0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15 };
    uint8_t co2 = tbl[co&15];
    if (co & 0x10)
        co2 = (co2 << 4); // & 0x7f;
    _cons_PRIVATE_col = co2;
}

/** Put string.
 */
void cons_puts(char const* s) {
    uint16_t offset = (_cons_PRIVATE_cur_y * s_textBufW + _cons_PRIVATE_cur_x);
    uint16_t co     = _cons_PRIVATE_col;
    while (*s) {
        s_textBuf[offset] = (co << 8) | *(uint8_t const*)s;
        ++s;
        ++offset;
        if (++_cons_PRIVATE_cur_x >= s_textBufW) {
            _cons_PRIVATE_cur_x = 0;
            if (++_cons_PRIVATE_cur_y >= s_textBufH) {
                _cons_PRIVATE_cur_y = 0;
            }
            offset = (_cons_PRIVATE_cur_y * s_textBufW);
        }
    }
}

/** Set position(x,y) and put string.
 */
void cons_xyputs(cons_pos_t x, cons_pos_t y, char const* s) {
    cons_setxy(x,y);
    cons_puts(s);
}

/** Set position(x,y), set color and put string.
 */
void cons_xycputs(cons_pos_t x, cons_pos_t y, cons_col_t c, char const* s) {
    cons_setxy(x,y);
    cons_setcolor(c);
    cons_puts(s);
}

/** static buffer sprintf
 */
char* _cons_PRIVATE_sprintf(char const* fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(s_cons_sprintf_buf, CONS_PRINTF_BUF_SIZE-1, fmt, arg);
    s_cons_sprintf_buf[CONS_PRINTF_BUF_SIZE-1] = 0;
    va_end(arg);
    return s_cons_sprintf_buf;
}
