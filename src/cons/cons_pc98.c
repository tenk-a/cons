/**
 *  @file cons_pc98.c
 *  @brief A console screen library for pc98 msdos.
 *  @author Masashi Kitamura ( https://github.com/tenk-a/ )
 *  @date   2024-12
 *  @license Boost Software License - Version 1.0
 */
#include "cons_pc98.h"
#include "dos_wrap.h"
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#define DPRINTF(...)
//#define DPRINTF(...)      do { printf(__VA_ARGS__); fflush(stdout); } while (0)
#define DBG_FL()            DPRINTF("%s %d\n", __FILE__, __LINE__)

#if defined(__DJGPP__)
 #undef  USE_VSYNC_INTR
 #define irq_disable()   __dpmi_get_and_disable_virtual_interrupt_state()   // 0x900
 #define irq_enable()    __dpmi_get_and_enable_virtual_interrupt_state()    // 0x901
#else   // watcom
 #define USE_VSYNC_INTR
 #if !defined(CONS_USE_NEAR_TEXT_BUF) && defined(__FLAT__)
  #define CONS_USE_NEAR_TEXT_BUF
 #endif
#endif


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/** Initialize video.
 */
static void video_init() {
    union REGS r = {0};
    r.h.ah = 0x03;
    int86(0x18, &r, &r);
}

/** Show/hide text.
 */
static void text_show(uint8_t flg) {
    union REGS r = {0};
    r.h.ah = (flg) ? 0x0c : 0x0d;
    int86(0x18, &r, &r);
}

/** Show/hide cursor.
 */
static void text_cursorSw(uint8_t show) {
    union REGS r = {0};
    r.h.ah = (show) ? 0x11 : 0x12;
    int86(0x18, &r, &r);
}

/** vblank start wait.
 */
static void vsync_wait(unsigned flags) {
    enum { STATUS_PORT = 0xA0 };

    if (flags & 1) {    // v-blank start
        while ((inp(STATUS_PORT) & 0x20)) {
            ;
        }
    }
    if (flags & 2) {    // v-blank end
        while (!(inp(STATUS_PORT) & 0x20)) {
            ;
        }
    }
}

static uint8_t s_textLineFnKey; //0060:0111: 0:none 1:omote func. 2:ura func.
static uint8_t s_textLine;      //0060:0112:  hight-1 (if s_fnKeyLine==0, add + 1)
static uint8_t s_textLineMode;  //0060:0113: 0:20line 1:25line

static void text_lineCheck(void) {
    s_textLineFnKey  = DOS_PEEKB(MK_FAR_PTR(0x0060,0x0111));
    s_textLine       = DOS_PEEKB(MK_FAR_PTR(0x0060,0x0112));
    s_textLineMode   = DOS_PEEKB(MK_FAR_PTR(0x0060,0x0113));
}

static void text_conPut(char const* s) {
    union REGS r = {0};
    r.h.ah = 0x06;
    while ((r.h.dl = *s++) != 0) {
        int86(0x21, &r, &r);
    }
}
static void text_PFKeySw(uint8_t f) {text_conPut((f)?"\x1b[>1l":"\x1b[>1h");}
static void text_cls() { text_conPut("\x1b[2J"); }


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
//  Key

/** Is a key pressed?
 */
static int key_kbHit(void) {
    union REGS r = {0};
    r.h.ah = 0x01;
    int86(0x18, &r, &r);
    return r.h.bh != 0;
}

/**
 */
static int key_wait(void) {
    union REGS r = {0};
    r.h.ah = 0x00;
    int86(0x18, &r, &r);
    return r.w.ax;
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
    union REGS r = {0};
    r.w.ax = 0x0cff;
    int86(0x21, &r, &r);
}


#if 0
static int key_scan(void) {
    union REGS r = {0};
    r.h.ah = 0x01;
    //  out ax　key code data
    //  out bh  0:disable 1:enable
    int86(0x18, &r, &r);
    if (r.h.bh == 0)
        return -1;
    return r.w.ax;
}

//  out al  b0:SHIFT  b1:CAPS  b2:KANA  b3:GRPH  b4:CTRL
static unsigned key_shift(void) {
    union REGS r = {0};
    r.h.ah = 0x02;
    int86(0x18, &r, &r);
    return r.h.al;
}

static int key_sence(uint8_t keyGrp) {
    union REGS r = {0};
    r.h.al = keyGrp;
    r.h.ah = 0x04;
    int86(0x18, &r, &r);
    return r.h.al;
}
#endif


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

// Text-Vram

#define TVRAM_TXT_SEG       0xA000
#define TVRAM_ATR_SEG       0xA200

#define TVRAM_W             80
//#define TVRAM_H           s_tvram_height  //25 //31
#define TVRAM_MAX_H         32
#define TVRAM_BUF_ATR_OFS   0x1000  // (TVRAM_W * TVRAM_H) ha dame datta.
#define TVRAM_BUF_BYTES     (TVRAM_BUF_ATR_OFS * sizeof(uint16_t))
#define TVRAM_BUF_ALC_BYTES (2 * TVRAM_BUF_BYTES)
#define TVRAM_ALIGN_BYTES   16  //256

#if defined(CONS_USE_NEAR_TEXT_BUF)
typedef uint16_t*           tvram_buf_t;
static long long            s_tvram_buff0[(TVRAM_W*TVRAM_MAX_H*sizeof(uint16_t) + sizeof(long long)-1 + TVRAM_ALIGN_BYTES) / sizeof(long long)];
#else
typedef uint16_t FAR*       tvram_buf_t;
static tvram_buf_t          s_tvram_buff0   = NULL;
#endif
static tvram_buf_t          tvram_backbuf   = NULL;
static uint8_t              s_tvram_height;

static void tvram_checkHeight(void) {
    text_lineCheck();
    s_tvram_height  = s_textLine + (s_textLineFnKey == 0);
    if (s_tvram_height < 15)
        s_tvram_height = 25;
    else if (s_tvram_height > TVRAM_MAX_H)
        s_tvram_height = TVRAM_MAX_H;
}

static void tvram_clear(unsigned ch, unsigned atr);
static void tvram_flush(unsigned top, unsigned lines);


static int tvram_init(void) {
    if (tvram_backbuf)
        return 1;

    tvram_checkHeight();

 #if !defined(CONS_USE_NEAR_TEXT_BUF)
    s_tvram_buff0 = (tvram_buf_t)FAR_MALLOC(TVRAM_BUF_ALC_BYTES);
    if (!s_tvram_buff0)
        return 0;
 #endif
    tvram_backbuf   = FAR_ALIGN_PTR(tvram_buf_t, s_tvram_buff0, TVRAM_ALIGN_BYTES);
    tvram_clear(' ', (7<<5)|1);
    tvram_flush(0, s_tvram_height);
    return 1;
}

static void tvram_term(void) {
 #if !defined(CONS_USE_NEAR_TEXT_BUF)
    FAR_FREE(s_tvram_buff0);
 #endif
    tvram_backbuf = NULL;
}


/** Screen refresh.
 */
static void tvram_flush(unsigned top, unsigned lines) {
    unsigned start, bytes;

    if (top >= TVRAM_MAX_H || lines == 0)
        return;
    if (lines > TVRAM_MAX_H - top)
        lines = TVRAM_MAX_H - top;

    start = top * TVRAM_W;
    bytes = TVRAM_W * lines * sizeof(uint16_t);

    DOS_MEMPUT(tvram_backbuf+start                  , bytes, MK_FAR_PTR(TVRAM_TXT_SEG, start));
    DOS_MEMPUT(tvram_backbuf+start+TVRAM_BUF_ATR_OFS, bytes, MK_FAR_PTR(TVRAM_ATR_SEG, start));
}

static void tvram_flushRect(unsigned x, unsigned y, unsigned w, unsigned h) {
    tvram_buf_t buf = tvram_backbuf;
    unsigned    wb  = w * sizeof(uint16_t);
    unsigned    ofs = (y * TVRAM_W + x) * sizeof(uint16_t);
    unsigned    n;
    for (n = 0; n < h; ++n) {
        DOS_MEMPUT(buf+(ofs>>1)                  , wb, MK_FAR_PTR(TVRAM_TXT_SEG, ofs));
        DOS_MEMPUT(buf+(ofs>>1)+TVRAM_BUF_ATR_OFS, wb, MK_FAR_PTR(TVRAM_ATR_SEG, ofs));
        ofs += TVRAM_W * sizeof(uint16_t);
    }
}

#define TVRAM_BUF_SET(buf, ofs, c, a)   (buf[ofs] = (c), buf[(ofs)+TVRAM_BUF_ATR_OFS] = (a))

/**
 */
static void tvram_clear(unsigned ch, unsigned atr) {
    tvram_buf_t buf     = tvram_backbuf;
    uint16_t    size    = TVRAM_W * s_tvram_height;
    uint16_t    offset  = 0;
    for (offset = 0; offset < size; ++offset) {
        TVRAM_BUF_SET(buf, offset, ch, atr);
    }
}


//  -   -   -   -   -   -   -   -   -   -
//  vsync

#define VSYNC_INT_VECT      0x0A      // INT 0Ah (IRQ2)
#define PIC_IMR_PORT        0x02      // 8259A IMR (master)
#define PIC_EOI_PORT        0x00      // 8259A EOI (master)
#define VSYNC_MASK_BIT      0x04
#define VSYNC_RESTART_PORT  0x64


#if defined(__FLAT__)
typedef uint64_t    vsync_t;
#else
typedef uint32_t    vsync_t;
#endif

#if defined(USE_VSYNC_INTR) == 0
static volatile vsync_t     s_vsync_count = 0;
static inline cons_clock_t  vsync_counterGet(void) {
    return ++s_vsync_count;
}
#define vsync_counterInit()
#define vsync_counterTerm()
#define vsync_update()

#elif defined(USE_VSYNC_INTR)

static volatile vsync_t     s_vsync_count       = 0;
static volatile uint16_t    s_vsync_delay_count = 0;
static uint16_t             s_vsync_delay       = 0;
static uint8_t              s_imr_save          = 0;
static DOS_VECT_ADR         s_vsync_handler_save;

// AH=41h INT 18h  bit2‑3 : 0x0C -> 31 kHz, else 24 kHz
static unsigned getGraphExtMode(void) {
    union REGS r = {0};
    r.w.ax = 0x4100;
    int86(0x18, &r, &r);
    return r.w.ax;
}


/** Get vsync-counter.
 */
static inline cons_clock_t vsync_counterGet(void) {
    outp(PIC_EOI_PORT      , 0x20);
    outp(VSYNC_RESTART_PORT, 0);
    return s_vsync_count;
}

/**
 */
#if defined(__DJGPP__)
static void vsync_handler(void)
#else
static void __interrupt __far __loadds vsync_handler()
#endif
{
    uint16_t tmp = s_vsync_delay_count + s_vsync_delay;
    if (tmp < s_vsync_delay_count)
        ++s_vsync_count;
    ++s_vsync_count;
    s_vsync_delay_count = tmp;

 #if !defined(__FLAT__)
    if (s_vsync_handler_save)
        (*s_vsync_handler_save)();
 #endif
    outp(PIC_EOI_PORT      , 0x20);
    outp(VSYNC_RESTART_PORT, 0);
}

/** Initialize vsync-counter.
 */
static void vsync_counterInit(void) {
    // https://github.com/koizuka/master.lib/blob/master/src/vsync.asm
    if ((getGraphExtMode() & 0x0C) == 0x0C) /* 31 kHz */
        s_vsync_delay = 13311;
    else
        s_vsync_delay = 0;
    s_vsync_count = 0;

    if (s_imr_save)
        return;

    s_vsync_handler_save = DOS_GETVECT(VSYNC_INT_VECT);
    DOS_SETVECT(VSYNC_INT_VECT, vsync_handler);

    _disable();
    s_imr_save = inp(PIC_IMR_PORT);
    outp(PIC_IMR_PORT, s_imr_save & ~VSYNC_MASK_BIT);
    //outp(0x60, inp(0x60) | 0x10);
    outp(PIC_EOI_PORT      , 0x20);
    outp(VSYNC_RESTART_PORT, 0);
    _enable();
}

/** Terminate vsync-counter.
 */
static void vsync_counterTerm(void) {
    if (!s_imr_save)
        return;

    _disable();
    //outp(0x60, inp(0x60) & ~0x10);
    outp(PIC_IMR_PORT, inp(PIC_IMR_PORT) | VSYNC_MASK_BIT);
    //_enable();
    //_disable();
    outp(PIC_IMR_PORT      , s_imr_save);
    outp(VSYNC_RESTART_PORT, 0);
    _enable();
    s_imr_save = 0;

    DOS_RESETVECT(VSYNC_INT_VECT, s_vsync_handler_save);
    DOS_CLEAR_VECT_ADR(s_vsync_handler_save);
}
#endif  // USE_VSYNC_INTR


// ================================================================

static cons_col_t       s_cur_col   = 0;

cons_clock_t            _cons_PRIVATE_tick;
cons_clock_t            _cons_PRIVATE_clock;
cons_key_t              _cons_PRIVATE_key;
cons_pos_t              _cons_PRIVATE_screen_width;
cons_pos_t              _cons_PRIVATE_screen_height;
cons_pos_t              _cons_PRIVATE_cur_x;
cons_pos_t              _cons_PRIVATE_cur_y;

typedef struct cons_rect_t {
    cons_pos_t  x, y;
    cons_pos_t  w, h;
} cons_rect_t;

static cons_rect_t      text_full_rect = { 0 };
static cons_rect_t      s_refresh_rect[CONS_REFRESH_RECT_N];
static char             s_cons_sprintf_buf[CONS_PRINTF_BUF_SIZE];
static char             s_has_fnLine   = 0;

static void cons_refresh(void);

/** Initialize.
 */
int  cons_init(unsigned flags) {
    (void)flags;
    text_cls();
    text_lineCheck();
    s_has_fnLine = s_textLineFnKey;

    video_init();
    text_show(0);
    text_PFKeySw(0);
    if (tvram_init() == 0)
        return 0;
    _cons_PRIVATE_screen_width  = TVRAM_W;
    _cons_PRIVATE_screen_height = s_tvram_height;
    text_full_rect.w            = TVRAM_W;
    text_full_rect.h            = s_tvram_height;

    vsync_counterInit();
    cons_setcolor(7);
    cons_clear();
    text_cursorSw(0);
    text_show(1);
    return 1;
}

/** Terminate.
 */
void cons_term(void) {
    vsync_counterTerm();
    s_refresh_rect[0] = text_full_rect;
    cons_setcolor(7);
    cons_clear();
    cons_refresh();
    tvram_term();
    text_cursorSw(1);
    text_PFKeySw(s_has_fnLine);
}

/** update-begin
 */
void cons_updateBegin(void) {
    memset(s_refresh_rect, 0, sizeof(s_refresh_rect));
    s_refresh_rect[0]     = text_full_rect;

    cons_setxy(0,0);
    cons_setcolor(7);
    _cons_PRIVATE_clock   = CONS_TICK_TO_MSEC(s_vsync_count);
    _cons_PRIVATE_tick    = vsync_counterGet();
    _cons_PRIVATE_key     = CONS_KEY_ERR;
    if (key_kbHit()) {
        _cons_PRIVATE_key = key_getch();
        key_bufClr();
    }
}

/** update-end
 */
void cons_updateEnd(void) {
    vsync_wait(1);
    cons_refresh();
    vsync_wait(2);
}

/** Screen clear.
 */
void cons_clear(void) {
    tvram_clear(' ', s_cur_col);
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
static void cons_refresh(void) {
    cons_rect_t const* rt = &s_refresh_rect[0];
    if (rt->w==TVRAM_W && rt->x==0) {
        tvram_flush(rt->y, rt->h);
    } else {
        cons_rect_t const* rt_e = &s_refresh_rect[CONS_REFRESH_RECT_N];
        do {
            int      x,y,w,h;
            x  = rt->x;
            w  = rt->w;
            if (x <= 0) {
                w += x;
                x = 0;
                if (w <= 0)
                    continue;
            } else if (x + w > TVRAM_W) {
                w = TVRAM_W - x;
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
            } else if (y + h > s_tvram_height) {
                h = s_tvram_height - y;
                if (h <= 0)
                    continue;
            }
            tvram_flushRect(x, y, w, h);
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

#define iskanji(c)  (((c)>=0x81 && (c)<=0x9f) || ((c)>=0xE0 && (c)<=0xfc))

/** SJIS->JIS
 */
static inline uint16_t sjisToJis(uint16_t ax) {
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

/** Put string.
 */
void cons_puts(char const* str) {
    tvram_buf_t     buf = tvram_backbuf;
    uint16_t        ofs = (_cons_PRIVATE_cur_y*TVRAM_W+_cons_PRIVATE_cur_x);
    uint8_t  const* s   = (uint8_t const*)str;

    while (*s) {
        uint8_t c = *s++;
        if (iskanji(c) == 0) {
            if (c == '\n') {
                _cons_PRIVATE_cur_x = TVRAM_W;
            } else {
                TVRAM_BUF_SET(buf, ofs, c, s_cur_col);
                ++ofs;
                ++_cons_PRIVATE_cur_x;
            }
        } else if (*s) {
            unsigned sjis = (c << 8) | *s++;
            uint16_t jis  = sjisToJis(sjis);
            uint8_t  ah   = (uint8_t)(jis);
            uint8_t  al   = (uint8_t)(jis >> 8);
            uint16_t ax;
            al -= 0x20;
            ax  = (ah << 8) | (al);
            TVRAM_BUF_SET(buf, ofs, ax       , s_cur_col);
            ++ofs;
            TVRAM_BUF_SET(buf, ofs, ax|0x8080, s_cur_col);
            ++ofs;
            _cons_PRIVATE_cur_x += 2;
        }
        if (_cons_PRIVATE_cur_x >= TVRAM_W) {
            _cons_PRIVATE_cur_x = 0;
            if (++_cons_PRIVATE_cur_y >= s_tvram_height)
                _cons_PRIVATE_cur_y = 0;
            ofs  = (_cons_PRIVATE_cur_y * TVRAM_W + _cons_PRIVATE_cur_x);
        }
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
