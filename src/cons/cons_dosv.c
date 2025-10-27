/**
 *  @file cons_dosv.c
 *  @brief Console screen library for DOS/V
 *  @author Masashi Kitamura ( https://github.com/tenk-a/ )
 *  @date  2025-09
 *  @license Boost Software License - Version 1.0
 */
#include "cons_dosv.h"
#include "dos_wrap.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include "dbg.h"

#ifndef USE_DOSV_TVRAM
#define USE_DOSV_TVRAM  1   // 2 は正常動作せず.
#endif

typedef uint16_t FAR* tvram_ptr_t;
enum tvram_type_t { TVT_NONE, TVT_BUF, TVT_PCAT, TVT_DOSV };

typedef struct cons_rect_t {
    cons_pos_t  x, y, w, h;
} cons_rect_t;

typedef struct cursor_info_t {
    uint8_t         startScanLine;
    uint8_t         endScanLine;
    uint8_t         cursorVisible;
} cursor_info_t;



//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

cons_clock_t            _cons_PRIVATE_clock;
cons_clock_t            _cons_PRIVATE_tick;
cons_key_t              _cons_PRIVATE_key;
cons_pos_t              _cons_PRIVATE_screen_width  = -1;
cons_pos_t              _cons_PRIVATE_screen_height = -1;
cons_pos_t              _cons_PRIVATE_cur_x;
cons_pos_t              _cons_PRIVATE_cur_y;
cons_col_t              _cons_PRIVATE_col;
unsigned                _cons_PRIVATE_act_cp;

static cursor_info_t    s_cursor_info;

static cons_clock_t     s_start_clock;
static uint8_t          s_save_video_mode;
static uint8_t          s_cur_video_mode;
static uint8_t          s_dosv_type;
static bool             s_enable_dosadr;

static bool             (*s_cp_leadbyte)(uint8_t c);

static uint8_t          s_tvram_type;
static cons_pos_t       s_tvram_w;
static cons_pos_t       s_tvram_h;
static tvram_ptr_t      s_tvram_bk;
static tvram_ptr_t      s_tvram_alc;
static unsigned         s_tvram_alc_seg;
static int              s_tvram_alc_selector;
static tvram_ptr_t      s_tvram;

static int              s_refresh_rects_idx = -1;
static cons_rect_t      s_refresh_rects[CONS_REFRESH_RECT_N];

static char             s_cons_sprintf_buf[CONS_PRINTF_BUF_SIZE];


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

#if defined(__WATCOMC__) && defined(__FLAT__)
// watcom では bp への設定のため intr,intrf を使ったが,
// watcom 32bit (というか dos4gw) では int 10h で ebp が正しく反映されず.
// dpmi 経由で int を使うと正しく bp が使われるようなので、そのようにした.
#pragma pack(push,1)
typedef struct RMREGS {
    union {
        struct { uint32_t edi, esi, ebp, _rsv, ebx, edx, ecx, eax; } x;
        struct { uint8_t  _di0,_di1,_di2,_di3, _si0, _si1, _si2, _si3,
                          _bp0,_bp1,_bp2,_bp3, _rv0, _rv1, _rv2, _rv3,
                          bl,bh,_b2,_b3,  dl,dh,_d2,_d3,
                          cl,ch,_c2,_c3,  al,ah,_a2,_a3; } h;
        struct { uint16_t di,_0, si,_1, bp,_2,_rv,_3,
                          bx,_4, dx,_5, cx,_6, ax,_7,
                          flags, es, ds, fs, gs, ip, cs, sp, ss; } w;
    };
} RMREGS;
#pragma pack(pop)

static int rmregs_intrf(uint8_t intr_no, RMREGS* rm) {
    union REGS   r  = {0};
    struct SREGS s  = {0};
    r.x.eax = 0x0300;
    r.h.bl  = intr_no;
    r.h.bh  = 0x00;
    s.es    = FP_SEG(rm);
    r.x.edi = FP_OFF(rm);
    int386x(0x31, &r, &r, &s);
    return rm->x.eax;
}
#undef INTR
#undef INTR_REGS
#define INTR        rmregs_intrf
#define INTR_REGS   RMREGS
#endif

/// Video モード取得.
///
static uint8_t sys_getVideoMode(void) {
    union REGS r;
    r.h.ah = 0x0F;
    int86(0x10, &r, &r);
    return r.h.al;
}

/// Video モード設定.
///
static void  sys_setVideoMode(uint8_t mode) {
    union REGS r;
    r.h.ah = 0x00;
    r.h.al = mode;
    int86(0x10, &r, &r);
}

/// テキストスクリーン横幅取得.
///
static int sys_getScreenWidth() {
 #if 1  // DOSワークメモリより取得.
    return DOS_PEEKB(MK_FAR_PTR(0x0040, 0x4a));
 #else
    union REGS r;
    r.h.ah = 0x0F;
    int86(0x10, &r, &r);
    return r.h.ah;
 #endif
}

/// テキストスクリーン縦幅取得.
///
static int sys_getScreenHeight() {
 #if 1  // DOSワークメモリより取得.
    int h = DOS_PEEKB(MK_FAR_PTR(0x0040, 0x84));
    // ++h;
    return h;
 #else  // bios経由だと高さがたまにへんな値に化ける...
    union REGS r;
    r.w.ax = 0x1130;
    int86(0x10, &r, &r);
    return r.h.dl;
 #endif
}

/// カーソル情報取得.
///
static void sys_getCursorInfo(cursor_info_t* ci) {
    union REGS r;
    r.h.ah = 0x03;
    r.h.bh = 0x00;
    int86(0x10, &r, &r);
    ci->startScanLine = r.h.ch;
    ci->endScanLine   = r.h.cl;
    ci->cursorVisible = (r.h.ch == 0x20) ? 0 : 1;
}

/// カーソル表示.
///
static void sys_showCursor(cursor_info_t const* ci) {
    union REGS r;
    r.h.ah = 0x01;
    r.h.ch = ci->startScanLine;
    r.h.cl = ci->endScanLine;
    int86(0x10, &r, &r);
}

/// カーソル表示オフ.
///
static void sys_hideCursor(void) {
    union REGS r;
    r.h.ah = 0x01;
    r.w.cx = 0x2000;
    int86(0x10, &r, &r);
}

/// カーソルブリンク on/off
///
static void sys_setBlinkMode(uint8_t sw) {
    union REGS r;
    r.w.ax = 0x1003;
    r.w.bx = sw;
    int86(0x10, &r, &r);
}

/// キーが押されたか.
///
static uint8_t sys_kbHit(void) {
    union REGS r;
    r.h.ah = 0x0B;
    int86(0x21, &r, &r);
    return r.h.al != 0;
}

/// キー1文字取得.
///
static uint16_t sys_getCh(void) {
    union REGS r;
    r.h.ah = 0x00;
    int86(0x16, &r, &r);
    if (r.h.al == 0 || r.h.al == 0xE0) {
        return (uint16_t)(0xE000 | r.h.ah);
    }
    return r.h.al;
}

enum { vga_status_port = 0x03DA };

/// V-ブランク開始を待つ.
///
static void sys_vblankBeginWait(void) {
    while ((inp(vga_status_port) & 0x08) != 0) { }
}

/// V-ブランクの終了を待つ.
///
static void sys_vblankEndWait(void) {
    while ((inp(vga_status_port) & 0x08) == 0) { }
}

/// 現在時間取得.
///
static cons_clock_t sys_getCurrentClock(void) {
 #if defined __DJGPP__
    return (cons_clock_t)(uclock() * CONS_CLOCK_PER_SEC / UCLOCKS_PER_SEC);
 #else
    return (cons_clock_t)(clock() * CONS_CLOCK_PER_SEC / CLOCKS_PER_SEC);
 #endif
}

/// 現在の言語 Code Page 取得.
///
static unsigned sys_getActiveCP(unsigned *default_cp) {
    union REGS r;
    r._W.ax = 0x6601;
    int86(0x21, &r, &r);
    if (r._W.cflag)
        return 0;
    if (default_cp)
        *default_cp = r._W.dx;
    return r._W.bx;
}

/// DOS/V での VIDEO バッファのアドレスを取得.
///
static tvram_ptr_t sys_dosvGetTextVideoBuffer(void) {
 #if defined(INTR_REGS)
    INTR_REGS  r = {0};
    r.h.ah = 0xFE;
    INTR(0x10, &r);
    return (tvram_ptr_t)MK_FAR_PTR(r._W.es, r._W.si);
 #else
    union  REGS  r = {0};
    struct SREGS s = {0};
    r.h.ah = 0xFE;
    int86x(0x10, &r, &r, &s);
    return (tvram_ptr_t)MK_FAR_PTR(s.es, r._W.si);
 #endif
}

/// DOS/V での VIDEO バッファ更新.
///
static void sys_dosvUpdateTextVideoBuffer(uint16_t seg, uint16_t ofs, unsigned num) {
 #if defined(INTR_REGS)
    INTR_REGS   r = {0};
    r.h.ah  = 0xFF;
    r._W.cx = num;
    r._W.es = seg;
    r._W.si = ofs;
    INTR(0x10, &r);
 #else
    union  REGS  r = {0};
    struct SREGS s = {0};
    r.h.ah  = 0xFF;
    r._W.cx = num;
    s.es    = seg;
    r._W.si = ofs;
    int86x(0x10, &r, &r, &s);
 #endif
}

/// DOS/V タイプを取得. 0:no 1:dos/v 2:j3100?
///
static int sys_getDosvType(void) {
    union  REGS  r = {0};
    r._W.ax = 0x4900;
    int86(0x15, &r, &r);
    if (r.h.ah == 0 && r._W.cflag == 0) {
        return r.h.bl + 1;
    } else {
        tvram_ptr_t tv = sys_dosvGetTextVideoBuffer();
        uint16_t    es = FAR_PTR_SEG(tv);
        return (es != 0xB800 && es != 0xB000 && es != 0x0000);
    }
}

/// テキスト&属性を(x,y)の位置に num 文字分表示.
///
static void sys_writeTextAttr(uint8_t x, uint8_t y, uint16_t num, uint16_t seg, uint16_t ofs) {
    INTR_REGS   r = {0};
    r._W.ax  = 0x1302;  // 0x1303
    r._W.bx  = 0x0000;
    r.h.dl   = (uint8_t)x;
    r.h.dh   = (uint8_t)y;
    r._W.cx  = (uint16_t)num;
    r._W.bp  = ofs;
    r._W.es  = seg;
    INTR(0x10, &r);
}

/// DOS メモリ(先頭 1MB以内) のメモリを取得する.
///
static unsigned sys_allocateDosMemory(unsigned bytes, int* selector) {
    uint16_t para = (bytes + 15) >> 4;
 #if defined(__DJGPP__)
    *selector = 0;
    return __dpmi_allocate_dos_memory(para, selector);
 #elif defined(__WATCOMC__) //&& defined(__FLAT__)
    unsigned seg = 0;
    if (_dos_allocmem(para, &seg) == 0) {
        *selector = (int)seg;
        return seg;
    }
    *selector = 0;
    return 0;
 #else
    union  REGS  r; // = {0};
    r._W.ax   = 0x4800;
    r._W.bx   = para;
    int86(0x21, &r, &r);
    if (r._W.cflag == 0) {
        *selector = r._W.ax;
        return r._W.ax;
    } else {
        *selector = 0;
        return 0;
    }
 #endif
}

/// sys_allocateDosMemory したメモリの解放.
///
static void sys_freeDosMemory(int selector) {
 #if defined(__DJGPP__)
    __dpmi_free_dos_memory(selector);
 #elif defined(__WATCOMC__) //&& defined(__FLAT__)
    _dos_freemem((unsigned)selector);
 #else
    union  REGS  r; // = {0};
    struct SREGS s; // = {0};
    r.h.ah = 0x49;
    s.es   = selector;
    int86x(0x21, &r, &r, &s);
 #endif
}


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
static void codepage_init(void);
static void tvram_init(uint8_t dosv_type);
static void tvram_term(void);
static void tvram_bk_clear(uint8_t ch, uint8_t atr);
static void tvram_alc_alloc(size_t bytes);
static void tvram_alc_free(void);
static void tvram_bk_realloc(uint8_t tvram_w, uint8_t tvram_h);
static void tvram_updRect_indirect(cons_pos_t x, cons_pos_t y, cons_pos_t w, cons_pos_t h);
static void tvram_updRect_indirectBuf(cons_pos_t x, cons_pos_t y, cons_pos_t w, cons_pos_t h);
static void tvram_updRect_direct(cons_pos_t x, cons_pos_t y, cons_pos_t w, cons_pos_t h);
static void tvram_updRect_directDosv(cons_pos_t x, cons_pos_t y, cons_pos_t w, cons_pos_t h);

static void consTvramUpdate(void);
static void consRefresh(void);

#if defined(__DJGPP__)
#define enable_dosadr()     (s_enable_dosadr)
#else
#define enable_dosadr()     1
#endif

/// 初期化.
///
int cons_init(unsigned flags) {
    (void)flags;
    codepage_init();
    s_enable_dosadr     = DOS_ADDR_INIT();
    _cons_PRIVATE_col   = CONS_COL_DEFAULT;
    s_save_video_mode   = sys_getVideoMode();
    s_cur_video_mode    = s_save_video_mode;

    s_dosv_type         = sys_getDosvType();
    if (s_dosv_type && s_cur_video_mode >= 0x70 && s_cur_video_mode <= 0x71) {
        s_cur_video_mode= 0x70;
    } else {
        s_cur_video_mode= 0x03;
    }
    if (s_cur_video_mode != s_save_video_mode)
        sys_setVideoMode(s_cur_video_mode);

    DBG_LOG("video_mode=%02x -> %02x\n", s_save_video_mode, s_cur_video_mode);

    sys_getCursorInfo(&s_cursor_info);
    sys_setBlinkMode(0);
    sys_hideCursor();

    tvram_init(s_dosv_type);
    consTvramUpdate();
    cons_clear();
    consRefresh();

    s_start_clock = sys_getCurrentClock();
    return 1;
}

/// 終了.
///
void cons_term(void) {
    cons_setRefreshRect(0, 0,0, s_tvram_w, s_tvram_h);
    tvram_bk_clear(' ', 7);
    consRefresh();
    tvram_alc_free();
    if (s_cursor_info.cursorVisible)
        sys_showCursor(&s_cursor_info);
    sys_setVideoMode(s_save_video_mode);
    DOS_ADDR_TERM();
}

/// 毎フレームの更新開始.
///
void cons_updateBegin(void) {
    cons_key_t k;

    consTvramUpdate();

    _cons_PRIVATE_clock = sys_getCurrentClock() - s_start_clock;
    _cons_PRIVATE_tick  = _cons_PRIVATE_clock * 60 / CONS_CLOCK_PER_SEC;

    if (sys_kbHit()) {
        do {
            k = sys_getCh();
        } while (sys_kbHit());
    } else {
        k = CONS_KEY_ERR;
    }
    _cons_PRIVATE_key = k;
}

/// TVRAM のサイズ変更確認.(バックバッファ取得)
///
static void consTvramUpdate(void) {
    cons_pos_t  tvram_w = sys_getScreenWidth();
    cons_pos_t  tvram_h = sys_getScreenHeight() + (s_tvram_type != TVT_BUF);
    if (tvram_w != s_tvram_w || tvram_h != s_tvram_h) {
        DBG_LOG("%s %d*%d -> %d*%d\n", __func__, s_tvram_w, s_tvram_h, tvram_w, tvram_h);
        tvram_init(s_dosv_type);
        tvram_bk_realloc(tvram_w, tvram_h);
        _cons_PRIVATE_screen_width  = tvram_w;
        _cons_PRIVATE_screen_height = tvram_h;
    }
    cons_setRefreshRect(0, 0,0, tvram_w, tvram_h);
}

/// 毎フレームの更新終了.
///
void cons_updateEnd(void) {
    sys_vblankBeginWait();
    consRefresh();
    sys_vblankEndWait();
}

/// 画面クリア.
///
void cons_clear(void) {
    consTvramUpdate();
    cons_setcolor(7);
    tvram_bk_clear(' ', _cons_PRIVATE_col);
    cons_setxy(0, 0);
}

/// 文字色設定.
///
void cons_setcolor(uint8_t co) {
    static const uint8_t tbl[16] = { 0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15 };
    uint8_t c = tbl[co & 15];
    if (co & 0x10)
        c = (uint8_t)(c << 4);
    _cons_PRIVATE_col = c;
}

/// 現在のカーソル位置に現在の色で、文字列表示(バックバッファへの書き込み)
///
void cons_puts(char const* str) {
    uint8_t const*  s   = (uint8_t const*)str;
    uint16_t        atr = (uint8_t)_cons_PRIVATE_col << 8;
    unsigned        x   = (unsigned)_cons_PRIVATE_cur_x;
    unsigned        y   = (unsigned)_cons_PRIVATE_cur_y;
    unsigned        dstw= (unsigned)s_tvram_w;
    tvram_ptr_t     dst = s_tvram_bk + y * dstw;
    while (*s) {
        uint8_t ch  = *s++;
        if (ch == '\n') {
            x = dstw;
        } else if (s_cp_leadbyte(ch)) {
            if (*s == 0)
                break;
            if (x+1 < dstw) {
                dst[x++] = (ch)   | atr;
                dst[x++] = (*s++) | atr;
            } else {
                --s;
                x = dstw;
            }
        } else {
            dst[x++] = (ch) | atr;
        }
        if (x >= dstw) {
            dst += dstw;
            x    = 0;
            if (++y >= (unsigned)s_tvram_h) {
                y   = 0;
                dst = s_tvram_bk;
            }
        }
    }
    _cons_PRIVATE_cur_x = (cons_pos_t)x;
    _cons_PRIVATE_cur_y = (cons_pos_t)y;
}

/// (x,y)位置にカーソル移動して文字列表示.
///
void cons_xyputs(cons_pos_t x, cons_pos_t y, char const* s) {
    cons_setxy(x,y);
    cons_puts(s);
}

/// (x,y)位置にカーソル移動して指定色で文字列表示.
///
void cons_xycputs(cons_pos_t x, cons_pos_t y, cons_col_t c, char const* s) {
    cons_setxy(x,y);
    cons_setcolor((uint8_t)c);
    cons_puts(s);
}

/// static な内部文字バッファに対する sprintf
///
char* _cons_PRIVATE_sprintf(char const* fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(s_cons_sprintf_buf, CONS_PRINTF_BUF_SIZE-1, fmt, arg);
    s_cons_sprintf_buf[CONS_PRINTF_BUF_SIZE-1] = 0;
    va_end(arg);
    return s_cons_sprintf_buf;
}

/// 今回のフレームでの部分的な画面更新範囲を設定.
///
void cons_setRefreshRect(uint8_t no, cons_pos_t x, cons_pos_t y, cons_pos_t w, cons_pos_t h) {
    if (no < CONS_REFRESH_RECT_N) {
        if (s_refresh_rects_idx <= no)
            s_refresh_rects_idx  = no;
        s_refresh_rects[no].x = x;
        s_refresh_rects[no].y = y;
        s_refresh_rects[no].w = w;
        s_refresh_rects[no].h = h;
    } else {
        assert(no < CONS_REFRESH_RECT_N);
    }
}

/// バックバッファの内容を表示に反映.
///
static void consRefresh(void) {
    cons_rect_t* rt   = &s_refresh_rects[0];
    cons_rect_t* rt_e;
    if (s_refresh_rects_idx < 0) {
     #if 1
        return;
     #else
        s_refresh_rects_idx = 0;
        rt->x = 0;
        rt->y = 0;
        rt->w = s_tvram_w;
        rt->h = s_tvram_h;
     #endif
    }
 #if USE_DOSV_TVRAM == 1 && defined(__WATCOMC__)
    if (s_tvram_type == TVT_DOSV) {
        sys_dosvUpdateTextVideoBuffer(FAR_PTR_SEG(s_tvram), FAR_PTR_OFF(s_tvram), s_tvram_w * s_tvram_h);
        return;
    }
 #endif
    rt_e = &s_refresh_rects[s_refresh_rects_idx + 1];
    do {
        cons_pos_t w = rt->w, h = rt->h;
        if (w && h) {
            cons_pos_t x = rt->x, y = rt->y;
            if (x < 0) {
                w += x;
                x = 0;
            }
            if (y < 0) {
                h += y;
                y = 0;
            }
            if (x >= s_tvram_w || y >= s_tvram_h)
                continue;
            if (x + w > s_tvram_w)
                w = s_tvram_w - x;
            if (y + h > s_tvram_h)
                h = s_tvram_h - y;
            if (w <= 0 || h <= 0)
                continue;
            switch (s_tvram_type) {
         #if USE_DOSV_TVRAM == 2 && defined(__WATCOMC__)
            case TVT_DOSV:  tvram_updRect_directDosv( x, y, w, h); break;
         #elif defined(__DJGPP__)
            case TVT_DOSV:  tvram_updRect_direct(     x, y, w, h); break;
         #endif
            case TVT_PCAT:  tvram_updRect_direct(     x, y, w, h); break;
            case TVT_BUF:   tvram_updRect_indirectBuf(x, y, w, h); break;
            default:        tvram_updRect_indirect(   x, y, w, h); break;
            }
        }
    } while (++rt < rt_e);
 #if USE_DOSV_TVRAM == 1 && defined(__DJGPP__)
    if (s_tvram_type == TVT_DOSV)
        sys_dosvUpdateTextVideoBuffer(FAR_PTR_SEG(s_tvram), FAR_PTR_OFF(s_tvram), s_tvram_w * s_tvram_h);
 #endif
    memset(&s_refresh_rects[0], 0, sizeof(s_refresh_rects));
    s_refresh_rects_idx = -1;
}


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
// language code page

/// 1バイト文字 (2バイト文字の1バイト目か?)
///
static bool sbc_leadbyte(uint8_t c) {
    (void)c;
    return 0;
}

/// 2バイト文字の1バイト目か? (sjis以外)
///
static bool dbc_leadbyte(uint8_t c) {
    //return (c >= 0x81 && c <= 0xFE);
    return (unsigned)(c - 0x81) <= (0xFE - 0x81);
}

/// SJIS 2バイト文字の1バイト目か?
///
static bool sjis_leadbyte(uint8_t c) {
    //return (0x81 <= c && c <= 0x9f) || (0xE0 <= c && c <= 0xFC);
    return ((unsigned)(c ^ 0x20) - 0xA1) < 0x3C;
}

/// 言語 CadePage の取得初期化.
///
static void codepage_init(void) {
    unsigned cp = sys_getActiveCP(NULL);
    if (cp == 932)
        s_cp_leadbyte   = sjis_leadbyte;
    else if (cp == 936 || cp == 949 || cp == 950)
        s_cp_leadbyte   = dbc_leadbyte;
    else
        s_cp_leadbyte   = sbc_leadbyte;
    _cons_PRIVATE_act_cp= cp;
    DBG_LOG("CP=%d\n", cp);
}


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/// テキストVRAM に関する初期化.
///
static void tvram_init(uint8_t dosv_type) {
    DBG_LOG("%s dosv_type=%d enable_dosadr=%d\n", __func__, dosv_type, enable_dosadr());
    if (enable_dosadr()) {
        s_tvram = sys_dosvGetTextVideoBuffer();
        DBG_LOG("tvram=%08lx\n", (uint32_t)(s_tvram));
        if ((dosv_type == 0 && s_tvram == 0) || s_tvram == (tvram_ptr_t)MK_FAR_PTR(0xB800,0)) {
            s_tvram      = (tvram_ptr_t)MK_FAR_PTR(0xB800,0);
            s_tvram_type = TVT_PCAT;
            DBG_LOG("%s AT s_tvram = %08lx\n", __func__, s_tvram);
            return;
        } else if (dosv_type && s_tvram) {
            s_tvram_type = TVT_DOSV;
            DBG_LOG("%s DOSV s_tvram = %08lx\n", __func__, s_tvram);
            return;
        } else {
            s_tvram_type = TVT_BUF;
            s_tvram      = NULL;
            DBG_LOG("%s BUF\n", __func__);
            return;
        }
    }
    s_tvram_type = TVT_NONE;
    s_tvram      = NULL;
    DBG_LOG("%s NONE\n", __func__);
}

/// テキスト バックバッファ用メモリの取得.
///
static void tvram_alc_alloc(size_t bytes) {
 #if defined(__DJGPP__)
    bool const is_dos_alloc = enable_dosadr() && (s_tvram_type == TVT_BUF);
 #else
    enum { is_dos_alloc = 1 }; //bool const is_dos_alloc = enable_dosadr();
 #endif
    s_tvram_alc_seg = 0;
    if (is_dos_alloc) {
        s_tvram_alc_seg = sys_allocateDosMemory(bytes, &s_tvram_alc_selector);
        if (s_tvram_alc_seg) {
            DBG_LOG("%s seg = %04x(%x) bytes=%u\n", __func__, s_tvram_alc_seg, s_tvram_alc_selector, bytes);
            s_tvram_alc = (tvram_ptr_t)DOS_ADDR_TO(MK_FAR_PTR(s_tvram_alc_seg,0));
        } else {
            DBG_LOG("%s no seg\n", __func__);
        }
    }
 #if defined(__FLAT__)
    if (!s_tvram_alc_seg) {
        s_tvram_alc = (tvram_ptr_t)FAR_MALLOC(bytes);
    }
 #endif
}

/// テキスト バックバッファ用メモリの解放.
///
static void tvram_alc_free(void) {
    if (s_tvram_alc_seg)
        sys_freeDosMemory(s_tvram_alc_selector);
 #if defined(__FLAT__)
    else if (s_tvram_alc)
        FAR_FREE(s_tvram_alc);
 #endif
    s_tvram_alc         = NULL;
    s_tvram_alc_seg     = 0;
    s_tvram_alc_selector= 0;
}

/// テキスト バックバッファをクリア.
///
static void tvram_bk_clear(uint8_t ch, uint8_t atr) {
#if 1
    unsigned    dstw = s_tvram_w;
    tvram_ptr_t dst  = s_tvram_bk;
    size_t      size = s_tvram_h * dstw;
    uint8_t     a    = (atr == 0) ? 7 : atr;
    uint16_t    ca   = ch | (a << 8);
 #if 0 //defined(__DJGPP__)
    if (s_tvram == s_tvram_bk && s_tvram) {
        uint32_t d    = (uint32_t)s_tvram;
        uint32_t dend = d + size * sizeof(uint16_t);
        uint16_t buf[1024];
        unsigned i;
        for (i = 0; i < 1024; ++i)
            buf[i] = ca;
        do {
            uint32_t bytes = dend - d;
            if (bytes > 1024*2)
                bytes = 1024*2;
            DOS_MEMPUT(buf, bytes, d);
            d += bytes;
        } while (d < dend);
        return;
    }
 #endif
    do {
        *dst++ = ca;
    } while (--size);
#endif
}

/// テキスト バックバッファ(再)取得.
///
static void tvram_bk_realloc(uint8_t tvram_w, uint8_t tvram_h) {
    tvram_alc_free();
 #if defined(__WATCOMC__)
    if (s_tvram_type == TVT_DOSV && s_tvram) {
        s_tvram_bk = (tvram_ptr_t)s_tvram;
        //s_tvram_bk = (tvram_ptr_t)DOS_ADDR_TO(s_tvram);
        DBG_LOG("%s dosv %08lx -> %08lx\n", __func__, s_tvram, s_tvram_bk);
    } else
 #endif
    {
        size_t bytes = tvram_w * tvram_h * 2u;
        tvram_alc_alloc(bytes);
        s_tvram_bk = s_tvram_alc;
        if (!s_tvram_bk) {
            tvram_w = 0;
            tvram_h = 0;
        }
        DBG_LOG("%s s_tvram_bk = %08lx bytes=%u\n", __func__, s_tvram_bk, bytes);
    }
    s_tvram_w   = tvram_w;
    s_tvram_h   = tvram_h;
    if (s_tvram_bk) {
        tvram_bk_clear(' ', 0x07);
    }
}

/// バックバッファの指定矩形を表示. bios のテキストバッファ書き込みを使用.
///
static void tvram_updRect_indirectBuf(cons_pos_t x, cons_pos_t y, cons_pos_t w, cons_pos_t h) {
    unsigned      seg     = s_tvram_alc_seg;
    if (seg) {
        unsigned  tvram_w = s_tvram_w;
        unsigned  ofs     = tvram_w * y + x;
        assert(w > 0 && h > 0);
        DBG_ONCE_LOG("%s %d,%d %4d*%-4d %4x:%04x\n", __func__, x,y,w,h, seg, ofs*2);
        if (w == tvram_w) {
            sys_writeTextAttr(x, y, w * h, seg, ofs*2);
        } else {
            do {
                sys_writeTextAttr(x, y, w, seg, ofs*2);
                ofs += tvram_w;
                ++y;
            } while (--h > 0);
        }
        return;
    }
    tvram_updRect_indirect(x,y,w,h);
}

/// バックバッファの指定矩形を表示. bios の1文字単位の表示を使用.
///
static void tvram_updRect_indirect(cons_pos_t x, cons_pos_t y, cons_pos_t w, cons_pos_t h) {
    tvram_ptr_t src     = s_tvram_alc;
    unsigned    tvram_w = s_tvram_w;
    unsigned    xend    = x + w;
    union REGS  r       = {0};
    assert(w > 0 && h > 0);
    src += tvram_w * y;
    do {
        uint16_t c;
        unsigned xx = x;
        do {
            r.h.ah  = 0x02;
            r.h.bh  = 0;
            r.h.dl  = xx;
            r.h.dh  = y;
            int86(0x10, &r, &r);
            r.h.ah  = 0x09;
            r._W.cx = 1;
            c       = src[xx];
            r.h.al  = (uint8_t)(c);
            r._W.bx = (uint8_t)(c >> 8);
            int86(0x10, &r, &r);
        } while (++xx < xend);
        src += tvram_w;
        ++y;
    } while (--h > 0);
}

/// バックバッファの指定矩形を表示. 直接 テキストVRAM を書き換える.
///
static void tvram_updRect_direct(cons_pos_t x, cons_pos_t y, cons_pos_t w, cons_pos_t h) {
    unsigned    tvram_w = (unsigned)s_tvram_w;
    size_t      ofs     = tvram_w * y + x;
    tvram_ptr_t src     = s_tvram_bk + ofs;
    //tvram_ptr_t dst   = (tvram_ptr_t)DOS_ADDR_TO(s_tvram) + ofs;
    uint32_t    dst     = (uint32_t)(s_tvram) + ofs * sizeof(uint16_t);
    assert(w > 0 && h > 0 && tvram_w > 0);
    if (w == tvram_w) {
        DOS_MEMPUT(src, w * h * sizeof(uint16_t), dst);
    } else {
        do {
            DOS_MEMPUT(src, w * sizeof(uint16_t), dst);
            src += tvram_w;
            dst += tvram_w*sizeof(uint16_t);
        } while (--h > 0);
    }
}

#if USE_DOSV_TVRAM == 2 //失敗.
/// バックバッファの指定矩形を表示. DOS/V テキストバッファの更新を行う ... フル画面以外は反映されず失敗.
///
static void tvram_updRect_directDosv(cons_pos_t x, cons_pos_t y, cons_pos_t w, cons_pos_t h) {
    unsigned    tvram_w = (unsigned)s_tvram_w;
    uint16_t    seg     = FAR_PTR_SEG(s_tvram);
    uint16_t    ofs     = FAR_PTR_OFF(s_tvram);
    ofs += (tvram_w * y + x) * sizeof(uint16_t);
    assert(w > 0 && h > 0 && tvram_w > 0 && s_tvram);
    if (w == tvram_w) {
        sys_dosvUpdateTextVideoBuffer(seg, ofs, w * h);
    } else {
        unsigned    add = tvram_w*2;
        do {
            sys_dosvUpdateTextVideoBuffer(seg, ofs, w);
            ofs += add;
        } while (--h > 0);
    }
}
#endif
