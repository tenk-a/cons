/**
 *  @file cons_pc98.c
 *  @brief A console screen library for pc98 msdos.
 *  @author Masashi Kitamura ( https://github.com/tenk-a/ )
 *  @date   2024-12
 *  @license Boost Software License - Version 1.0
 */
#include "cons_pc98.h"

#if defined(__WATCOMC__)
#define __WATCOM_PC98__
#include <bios.h>
#endif
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

#define CONS_USE_SJIS
//#define CONS_USE_NEAR_TEXT_BUF

#if defined __FLAT__    // 32bit DOS
#undef __far
#define __far
#define MK_FAR_PTR(a,b) ((a << 4) + (b))
#define _fmalloc        malloc
#define _ffree          free
#define int86           int386
#else                   // 16bit DOS
#define MK_FAR_PTR(a,b) (((uint32_t)(a) << 16) | (b))
#define FAR_PTR_SEG(p)  ((uint16_t)((uint32_t)(void __far*)(p) >> 16))
#define FAR_PTR_OFF(p)  ((uint16_t)((uint32_t)(p)))
#endif

#define TEXT_VRAM       ((uint16_t __far*)MK_FAR_PTR(0xA000,0x0000))
#define ATTR_VRAM       ((uint16_t __far*)MK_FAR_PTR(0xA200,0x0000))
#define TEXT_BUF_W      80
#define TEXT_BUF_H      25
#define TEXT_BUF_BYTES  (TEXT_BUF_W * TEXT_BUF_H * sizeof(uint16_t))

//extern unsigned char __isPC98;    // watcom

#if !defined(CONS_USE_NEAR_TEXT_BUF)
static uint16_t __far*  s_textBuf   = NULL;
static uint16_t __far*  s_attrBuf   = NULL;
#else
typedef struct aln_t {
    void*   p;
    double  d;
} aln_t;
static aln_t            s_buff[(2*TEXT_BUF_BYTES + 16) / sizeof(aln_t)];
static uint16_t*        s_textBuf = NULL;
static uint16_t*        s_attrBuf = NULL;
#endif

static cons_col_t       s_cur_col   = 0;

cons_clock_t            _cons_PRIVATE_tick;
cons_clock_t            _cons_PRIVATE_clock;
cons_key_t              _cons_PRIVATE_key;
cons_pos_t              _cons_PRIVATE_screen_width  = TEXT_BUF_W;
cons_pos_t              _cons_PRIVATE_screen_height = TEXT_BUF_H;
cons_pos_t              _cons_PRIVATE_cur_x;
cons_pos_t              _cons_PRIVATE_cur_y;

typedef struct cons_rect_t {
    cons_pos_t  x, y;
    cons_pos_t  w, h;
} cons_rect_t;

static cons_rect_t const text_full_rect = { 0,0,TEXT_BUF_W,TEXT_BUF_H };
static cons_rect_t      s_refresh_rect[CONS_REFRESH_RECT_N];
static char             s_cons_sprintf_buf[CONS_PRINTF_BUF_SIZE];


/** Initialize video.
 */
static void video_init() {
    union REGS regs;
    regs.h.ah = 0x03;
    int86(0x18, &regs, &regs);
}

/** Show/hide text.
 */
static void text_show(uint8_t flg) {
    union REGS regs;
    regs.h.ah = (flg) ? 0x0c : 0x0d;
    int86(0x18, &regs, &regs);
}

/** Show/hide cursor.
 */
static void text_cursorSw(uint8_t show) {
    union REGS regs;
    regs.h.ah = (show) ? 0x11 : 0x12;
    int86(0x18, &regs, &regs);
}

#if 0
static void text_conPut(char const* s) {
    union REGS regs = {0};
    regs.h.ah = 0x06;
    while ((regs.h.dl = *s++) != 0) {
        intdos(&regs, &regs);
    }
}
//static void text_PFKeySw(uint8_t f) {text_ConPut((f)?"\[[>1l":"\[[>1h");}
//static void text_cls() { text_ConPut("\[[2J"); }
#endif


//  -   -   -   -   -   -   -   -   -   -
//  Key

/** Is a key pressed?
 */
static int  key_kbHit(void) {
    union REGS regs;
    regs.h.ah = 0x01;
    int86(0x18, &regs, &regs);
    return regs.h.bh != 0;
}

/**
 */
static int key_wait(void) {
    union REGS regs;
    regs.h.ah = 0x00;
    int86(0x18, &regs, &regs);
    return regs.w.ax;
}

/** One key input
 */
static int key_getch(void) {
    int     c = key_wait();
    uint8_t b = (uint8_t)c;
    if (b >= ' ' && b <= 0x7e)
        c = b;
    return c;
}

/** Clear key buffer.
 */
static void key_bufClr(void) {
    union REGS regs;
    regs.w.ax = 0x0cff;
    intdos(&regs, &regs);
}


#if 0
static int key_scan(void) {
    union REGS regs;
    regs.h.ah = 0x01;
    //  out ax　key code data
    //  out bh  0:disable 1:enable
    int86(0x18, &regs, &regs);
    if (regs.h.bh == 0)
        return -1;
    return regs.x.ax;
}

//  out al  b0:SHIFT  b1:CAPS  b2:KANA  b3:GRPH  b4:CTRL
static unsigned key_shift(void) {
    union REGS regs;
    regs.h.ah = 0x02;
    int86(0x18, &regs, &regs);
    return regs.h.al;
}

static int key_sence(unsigned char keyGrp) {
    union REGS regs;
    regs.h.al = keyGrp;
    regs.h.ah = 0x04;
    int86(0x18, &regs, &regs);
    return regs.h.al;
}
#endif


//  -   -   -   -   -   -   -   -   -   -
//  vsync

/** vblank start wait.
 */
static void vsync_wait(void) {
    enum { statusPort = 0xA0 };
    while ((inp(statusPort) & 0x20)) {
        ;
    }
    while (!(inp(statusPort) & 0x20)) {
        ;
    }
}

enum { VSYNC_INTR = 0x0a };

static volatile uint32_t        s_vsync_count = 0;
static void (__interrupt __far *s_vsync_handler_old)(void) = NULL;

/**
 */
static void __interrupt __far vsync_handler(void) {
    ++s_vsync_count;
    if (s_vsync_handler_old)
        (*s_vsync_handler_old)();
    outp(0x64, 0);
}

/** Initialize vsync-counter.
 */
static void vsync_counterInit(void) {
     unsigned char mask;
    _disable();
    mask = inp(0x02);
    mask &= 0xFB;
    outp(0x02, mask);
    outp(0x64, 0);
    _enable();

    s_vsync_handler_old = _dos_getvect(VSYNC_INTR);
    _dos_setvect(VSYNC_INTR, vsync_handler);
}

/** Terminate vsync-counter.
 */
static void vsync_counterTerm(void) {
    if (s_vsync_handler_old) {
        _disable();
        _dos_setvect(VSYNC_INTR, s_vsync_handler_old);
        _enable();
        s_vsync_handler_old = NULL;
    }
}

/** Get vsync-counter.
 */
static cons_clock_t vsync_counterGet(void) {
 #if 1
    return s_vsync_count;
 #else
    static cons_clock_t s_clock = 0;
    return ++s_clock;
 #endif
}

//  -   -   -   -   -   -   -   -   -   -
// Timer

#if !defined(__FLAT__)
#define USE_T10MS
#endif

#if defined(USE_T10MS)

static volatile unsigned long s_t10ms_count = 0;

#if 1
static void t10ms_init(void);

/// Interval timer handler.
static void interrupt t10ms_handler(void) {
    ++s_t10ms_count;
    _enable();
    t10ms_init();
}

/// Timer start.
static void t10ms_init(void) {
 #if !defined(__FLAT__)
    union  REGS  regs;
    struct SREGS sregs;
    regs.h.ah = 0x02;
    regs.w.cx = 1;    // 1 => 10ms.
    regs.w.bx = FAR_PTR_OFF(t10ms_handler);
    sregs.es  = FAR_PTR_SEG(t10ms_handler);
    int86x(0x1C, &regs, &regs, &sregs);
 #elif 1
    union  REGS  regs  = {0};
    regs.h.ah  = 0x02;
    regs.w.cx  = 1;
    regs.x.ebx = (unsigned int)t10ms_handler;
    int86(0x1C, &regs, &regs);
 #endif
}

/// Timer stop.
static void t10ms_term(void) {
}
#else
static void interrupt (*t10ms_handler_old)(void) = NULL;

static void t10ms_init(void);
enum { T10MS_INTR = 7 };

///
static void interrupt t10ms_handler(void) {
    ++s_t10ms_count;
    if (t10ms_handler_old) {
        (*t10ms_handler_old)();
    }
}

/// Timer start.
static void t10ms_init(void) {
    s_t10ms_count     = 0;
    _disable();
    t10ms_handler_old = _dos_getvect(T10MS_INTR);
    _dos_setvect(T10MS_INTR, t10ms_handler);
    _enable();
}

/// Timer stop.
static void t10ms_term(void) {
    if (t10ms_handler_old) {
        _disable();
        _dos_setvect(T10MS_INTR, t10ms_handler_old);
        _enable();
    }
}
#endif

/// Get milli-seconds.
static unsigned long t10ms_getMilliSec(void) {
    return s_t10ms_count * 10;
}
#else
static void t10ms_init(void) {}
static void t10ms_term(void) {}
/// Get milli-seconds.
static unsigned long t10ms_getMilliSec(void) {
    return s_vsync_count * 1000 / 60;
}
#endif


// ================================================================

static void consRefresh(void);

/** Initialize.
 */
int  cons_init(unsigned flags) {
    (void)flags;
    video_init();
    text_show(1);
    text_cursorSw(0);
    //text_PFKeySw(0);
    if (!s_textBuf) {
      #if !defined(CONS_USE_NEAR_TEXT_BUF)
        size_t size = TEXT_BUF_BYTES;
        s_textBuf   = (uint16_t __far*)_fmalloc(2 * size * sizeof(uint16_t));
        s_attrBuf   = s_textBuf + size;
        _fmemset(s_textBuf, 0, 2 * size * sizeof(uint16_t));
      #else
        s_textBuf = (uint16_t*)(((uintptr_t)s_buff + 15) & ~15);
        s_attrBuf = s_textBuf + TEXT_BUF_W * TEXT_BUF_H;
      #endif
    }
    _fmemset(TEXT_VRAM, 0, TEXT_BUF_BYTES);
    _fmemset(ATTR_VRAM, 0, TEXT_BUF_BYTES);
    vsync_counterInit();
    t10ms_init();
    cons_setcolor(7);
    cons_clear();
    consRefresh();
    return 1;
}

/** Terminate.
 */
void cons_term(void) {
    t10ms_term();
    vsync_counterTerm();
    s_refresh_rect[0] = text_full_rect;
    cons_clear();
    consRefresh();
    text_cursorSw(1);
    //text_PFKeySw(1);
  #if !defined(CONS_USE_NEAR_TEXT_BUF)
    _ffree(s_textBuf);
  #endif
    s_textBuf = NULL;
    s_attrBuf = NULL;
}

/** update-begin
 */
void cons_updateBegin(void) {
    memset(s_refresh_rect, 0, sizeof(s_refresh_rect));
    s_refresh_rect[0] = text_full_rect;

    cons_setxy(0,0);
    cons_setcolor(7);
    _cons_PRIVATE_tick    = vsync_counterGet();
    _cons_PRIVATE_clock   = t10ms_getMilliSec();
    _cons_PRIVATE_key     = CONS_KEY_ERR;
    if (key_kbHit()) {
        _cons_PRIVATE_key = key_getch();
        key_bufClr();
    }
}

/** update-end
 */
void cons_updateEnd(void) {
    vsync_wait();
    consRefresh();
}

/** Screen clear.
 */
void cons_clear(void) {
    unsigned short offset = 0;
    unsigned x, y;
    for (y = 0; y < TEXT_BUF_H; ++y) {
        for (x = 0; x < TEXT_BUF_W; ++x) {
            s_textBuf[offset] = ' ';
            s_attrBuf[offset] = s_cur_col;
            ++offset;
        }
    }
    cons_setxy(0, 0);
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

/** Screen refresh.
 */
static void consRefresh(void) {
    cons_rect_t const* rt = &s_refresh_rect[0];
    if (rt->w==TEXT_BUF_W && rt->h==TEXT_BUF_H && rt->x==0 && rt->y==0) {
        _fmemcpy(TEXT_VRAM, s_textBuf, TEXT_BUF_BYTES);
        _fmemcpy(ATTR_VRAM, s_attrBuf, TEXT_BUF_BYTES);
    } else {
        cons_rect_t const* rt_e = &s_refresh_rect[CONS_REFRESH_RECT_N];
        do {
            unsigned n, ofs, bytes;
            int      x,y,w,h;
            x  = rt->x;
            w  = rt->w;
            if (x <= 0) {
                w += x;
                x = 0;
                if (w <= 0)
                    continue;
            } else if (x + w > TEXT_BUF_W) {
                w = TEXT_BUF_W - x;
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
            } else if (y + h > TEXT_BUF_H) {
                h = TEXT_BUF_H - y;
                if (h <= 0)
                    continue;
            }
            bytes = w * sizeof(uint16_t);
            ofs   = y * TEXT_BUF_W + x;
            for (n = 0; n < h; ++n) {
                _fmemcpy(&TEXT_VRAM[ofs], &s_textBuf[ofs], bytes);
                _fmemcpy(&ATTR_VRAM[ofs], &s_attrBuf[ofs], bytes);
                ofs += TEXT_BUF_W;
            }
        } while (++rt < rt_e);
    }
}

/** color to attr.
 * bit4:rev bit3:lig bit2:G bit1:R bit0:B
 */
static unsigned colToAtr(uint16_t col) {
    unsigned atr = (col & 0x10) >> 2;   // 0x10=rev
    col &= 7;
    return (col << 5) | atr | 1;
}

/** Set Color.
 */
void cons_setcolor(cons_col_t atr_col) {
    s_cur_col = colToAtr(atr_col);
}

/** Set position(x,y) and put string.
 */
void cons_xycputs(cons_pos_t x, cons_pos_t y, cons_col_t co, char const* s) {
    cons_setcolor(co);
    cons_setxy(x,y);
    cons_puts(s);
}

/** Set position(x,y) and put string.
 */
void cons_xyputs(cons_pos_t x, cons_pos_t y, char const* s) {
    cons_setxy(x,y);
    cons_puts(s);
}

#if defined(CONS_USE_SJIS)
#define iskanji(c)  (((c)>=0x81 && (c)<=0x9f) || ((c)>=0xE0 && (c)<=0xfc))

/** SJIS->JIS
 */
static uint16_t sjisToJis(uint16_t ax) {
    uint8_t al;
    uint8_t ah = ax >> 8;
    if (ah >= 0xa0)
        ah -= 0x40;
    ah -= 0x70;
    al = (uint8_t)ax;
    if ((int8_t)al < 0)    // al >= 0x80
        --al;
    ah <<= 1;
    if (al < 0x9e)
        --ah;
    else
        al -= 0x5e;
    al -= 0x1f;
    return (ah << 8) | al;
}
#endif

/** Put string.
 */
void cons_puts(char const* str) {
    uint8_t  const* s = (uint8_t const*)str;
    uint16_t offs     = (_cons_PRIVATE_cur_y*TEXT_BUF_W+_cons_PRIVATE_cur_x);

    while (*s) {
     #if defined(CONS_USE_SJIS)
        uint8_t c = *s++;
        if (iskanji(c) == 0) {
            if (c == '\n') {
                _cons_PRIVATE_cur_x = TEXT_BUF_W;
            } else {
                s_textBuf[offs] = c;
                s_attrBuf[offs] = s_cur_col;
                ++offs;
                ++_cons_PRIVATE_cur_x;
            }
        } else if (*s) {
            unsigned sjis = (c << 8) | *s++;
            uint16_t jis  = sjisToJis(sjis);
            uint8_t  ah   = (uint8_t)(jis);
            uint8_t  al   = (uint8_t)(jis >> 8);
            uint16_t ax;
            al -= 0x20;
            ax = (ah << 8) | (al);
            s_textBuf[offs+0] = ax;
            s_textBuf[offs+1] = ax | 0x8080;
            s_attrBuf[offs+0] = s_cur_col;
            s_attrBuf[offs+1] = s_cur_col;
            offs += 2;
            _cons_PRIVATE_cur_x += 2;
        }
        if (_cons_PRIVATE_cur_x >= TEXT_BUF_W) {
            _cons_PRIVATE_cur_x = 0;
            if (++_cons_PRIVATE_cur_y >= TEXT_BUF_H)
                _cons_PRIVATE_cur_y = 0;
            offs = (_cons_PRIVATE_cur_y * TEXT_BUF_W + _cons_PRIVATE_cur_x);
        }
      #else
        uint8_t c = *s++;
        if (c == '\n') {
            _cons_PRIVATE_cur_x = TEXT_BUF_W;
        } else {
            s_textBuf[offs] = c;
            s_attrBuf[offs] = s_cur_col;
            ++offs;
            ++_cons_PRIVATE_cur_x;
        }
      #endif
    }
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
