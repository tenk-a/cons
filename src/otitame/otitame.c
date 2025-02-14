/**
 *  @file   otitame.c
 *  @brief  落ちゲー始祖の変種.
 *  @author tenk* ( https://github.com/tenk-a )
 *  @date   2024-12
 *  @license Boost Software License - Version 1.0
 *  @note
 *   pdcurses/ncurses、pc-at dos, pc98 dos 用.
 */

#include "cons/cons.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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

#if defined(CONS_CURSES)
#define USE_SELECT_PIECE
#endif
//#define MOTO_GAME

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
//  etc

typedef unsigned int    uint_t;
typedef cons_pos_t      pos_t;

#define FIELD_W         10
#define FIELD_H         20

#if 0 //defined(__PCAT__)
#define CONSINIT_FLAGS  1
#else
#define CONSINIT_FLAGS  0
#endif

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
//  PIECE

#define PIECE_SHAPE_NUM   7     ///< ピース形状の種類数.

/// ピース形状 （7種類 × 4回転） src/tool/gen_shape.cpp
static uint16_t const piece_shapes[PIECE_SHAPE_NUM][4] = {
    //  0       90     180     270
    { 0x0660, 0x0660, 0x0660, 0x0660, },    // ■ O
    { 0x0c60, 0x2640, 0x0c60, 0x2640, },    // z  Z
    { 0x06c0, 0x4620, 0x06c0, 0x4620, },    // s  S
    { 0x8e00, 0x6440, 0x0e20, 0x44c0, },    // ┛ J
    { 0x2e00, 0x4460, 0x0e80, 0xc440, },    // ┗ L
    { 0x04e0, 0x4640, 0x0e40, 0x4c40, },    // ┻ T
    { 0x0f00, 0x2222, 0x0f00, 0x4444, },    // ┃ I
};

typedef struct Piece {
    pos_t       x;              ///< Field 内x位置.
    pos_t       y;              ///< Field 内y位置.
    uint8_t     shape;          ///< 形状 shape 種類(0〜6)
    uint8_t     r;              ///< 回転 rotate (0:0,1:90,2:180,3:270)
} Piece;

static Piece   s_piece_cur;     ///< 現在のピース.
static Piece   s_piece_next;    ///< 次のピース.

/// ピース初期化.
///
static void piece_init(Piece* p) {
    uint_t i;
    do {    // PIECE_SHAPE_NUM=7 に依存した 0..6乱数生成.
        i = rand();
        i = (i >> (14-3)) & 7;
    } while (i == 7);
    p->x     = (FIELD_W / 2) - 2;
    p->y     = -1;              // 各ピース 0°は上1行空白なので詰める.
    p->r     = 0;
    p->shape = i;
}


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
//  FIELD

typedef uint8_t         field_t;
static field_t          s_field[FIELD_H][FIELD_W];

#define field_get(x,y)  (s_field[y][x])

/// フィールド・クリア.
///
static void field_clear(void) {
    memset(s_field, 0, sizeof(s_field));
}

/// ピースを置けるか?
///
static bool field_canPlacePiece(Piece const* p) {
    uint16_t ptn = piece_shapes[p->shape][p->r];
    pos_t    x0  = p->x, y0 = p->y;
    uint8_t  i;
    for (i = 0; i < 16; ++i) {
        pos_t y = y0 + (i >> 2);
        if (y >= 0) {
            field_t const* field = s_field[y];
            if (ptn & (0x8000 >> i)) {
                pos_t x = x0 + (i &  3);
                if (x < 0 || x >= FIELD_W || y >= FIELD_H || field[x])
                    return 0;
            }
        }
    }
    return 1;
}

/// ピースの固定.
///
static void field_placePiece(Piece const* p) {
    uint16_t ptn = piece_shapes[p->shape][p->r];
    pos_t    x0  = p->x, y0 = p->y;
    uint8_t  i;
    for (i = 0; i < 16; ++i) {
        if (ptn & (0x8000 >> i)) {
            pos_t x = x0 + (i &  3);
            pos_t y = y0 + (i >> 2);
            if (y >= 0 && y < FIELD_H && x >= 0 && x < FIELD_W)
                s_field[y][x] = p->shape + 1;
        }
    }
}

#if !defined(MOTO_GAME)

/// 行が揃ったか?
/// @return 今回揃ったライン数.
static uint8_t filed_checkReach() {
    uint8_t lines = 0;
    pos_t   x,  y = FIELD_H;
    while (--y >= 0) {
        field_t* field = s_field[y];
        for (x = 0; x < FIELD_W; ++x) {
            field_t f = field[x];
            if (!f || (f & 0x8)) {
                field = NULL;
                break;
            }
        }
        if (field) {
            for (x = 0; x < FIELD_W; ++x)
                field[x] |= 8;
            ++lines;
        }
    }
    return lines;
}

/// ライン消去.
/// @return 消去したライン数.
static uint8_t filed_clearLines() {
    uint8_t lines = 0;
    pos_t   x,  y = FIELD_H, prev_y = FIELD_H;
    while (--y >= 0) {
        field_t* field = s_field[y];
        if (lines == 0)
            ;
        else if (y != prev_y)
            break;
        for (x = 0; x < FIELD_W; ++x) {
            field_t f = field[x];
            if (!(f & 0x8)) {
                field = NULL;
                break;
            }
        }
        if (field) {
            pos_t y2 = y;
            while (--y2 >= 0)
                memcpy(s_field[y2+1], s_field[y2], FIELD_W * sizeof(field_t));
            memset(s_field[0], 0, FIELD_W * sizeof(field_t));
            ++lines;
            prev_y = y;
            ++y;
        }
    }
    return lines;
}

#else   // MOTO_GAME

/// 元ゲー:ライン消去.
/// @return 消去したライン数.
static uint8_t filed_clearLinesMoto(void) {
    uint8_t lines = 0;
    pos_t   x,  y = FIELD_H;
    while (--y >= 0) {
        field_t* field1 = s_field[y];
        for (x = 0; x < FIELD_W; ++x) {
            if (!field1[x]) {
                field1 = NULL;
                break;
            }
        }
        if (field1) {
            pos_t y2 = y;
            while (--y2 >= 0)
                memcpy(s_field[y2+1], s_field[y2], FIELD_W * sizeof(field_t));
            memset(s_field[0], 0, FIELD_W * sizeof(field_t));
            ++lines;
            ++y;
        }
    }
    return lines;
}
#endif


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
//  GAME

#define GAME_MIN_SPEED   CONS_MSEC_TO_CLOCK(50)  ///< 最小速度(ミリ秒)

typedef enum GameState {
    GAME_EXIT   = 0,
    GAME_TITLE  = 1,
    GAME_START  = 2,
    GAME_PLAY   = 3,
    GAME_OVER   = 4,
} GameState;

static GameState s_cur_state  = GAME_TITLE; ///< 現在のステート.
static GameState s_next_state = GAME_TITLE; ///< 次回のステート.
static GameState s_prev_state = GAME_EXIT;  ///< 前回のステート.

static cons_clock_t s_fall_time= 0;         ///< 次の落下予定時間.
static uint_t   s_lines       = 0;          ///< クリアしたライン数.
static uint_t   s_pre_lines   = 0;          ///< 揃ったライン数.
static uint_t   s_level       = 1;          ///< レベル.
static uint_t   s_score       = 0;          ///< スコア.
static uint_t   s_high_score  = 0;          ///< ハイスコア.
static uint_t   s_speed       = 0;          ///< 落下速度.
static uint8_t  s_step        = 0;          ///< そのステートでのstep.
static uint8_t  s_choise      = 0;          ///< 選択子番号.

typedef enum DrawFlag {
    DRAWF_FIELD = 0x01,
    DRAWF_NEXT  = 0x02,
    DRAWF_INFO  = 0x04,
    DRAWF_TEXT  = 0x08,
    DRAWF_OVER  = 0x10,
    DRAWF_ALL   = 0x1F,
} DrawFlag;
static uint8_t  s_draw_flags  = 0;

static bool     gameUpdate(void);
static bool     gameTitle(void);
static bool     gameStart(void);
static bool     gamePlay(void);
static uint8_t  gameOver(void);
#if !defined(MOTO_GAME)
static void     pieceLand(void);
static void     pieceClear(void);
#else
static void     pieceLandMoto(void);
#endif
static void     checkLevelUp(void);
static void     draw_gameUpdate(void);
#if defined(USE_SELECT_PIECE)
static void     select_piece_init(int piece_stype);
#endif

enum { key_up=1, key_down, key_left, key_right, key_1, key_2, key_cancel };

/// キー入力.
///
static uint8_t getKey(void) {
    switch (cons_key()) {
    case CONS_KEY_UP:    case 'w': case 'W': return key_up;
    case CONS_KEY_DOWN:  case 's': case 'S': return key_down;
    case CONS_KEY_LEFT:  case 'a': case 'A': return key_left;
    case CONS_KEY_RIGHT: case 'd': case 'D': return key_right;
    case CONS_KEY_SPACE: case 'z': case 'Z': return key_1;
    case CONS_KEY_RETURN:case 'x': case 'X': return key_2;
    case CONS_KEY_ESC:   case 'c': case 'C': return key_cancel;
    default: return 0;
    }
}

/// ゲーム・メインループ.
/// @return osへ返す値. 0:正常終了. 1:エラー終了.
static int gameMain(void) {
    bool rc = 0;
    srand((int)time(NULL));         // 乱数初期化.
    if (!cons_init(CONSINIT_FLAGS)) // cons:コンソール画面初期化.
        return 1;
    do {
        cons_updateBegin();         // cons:画面の毎フレームの開始処理.
        rc = gameUpdate();          // ゲームの毎フレームの更新.
        draw_gameUpdate();          // ゲーム描画の毎フレームの更新.
        cons_updateEnd();           // cons:画面の毎フレーム終わりの処理.
    } while (rc);
    cons_term();                    // cons:コンソール画面終了処理.
    return 0;
}

/// ゲームの毎フレームの更新.
/// @return  0:終了 1:継続.
static bool gameUpdate(void) {
    uint8_t rc = 0;
    s_prev_state = s_cur_state;
    s_cur_state  = s_next_state;
    if (s_cur_state != s_prev_state)
        s_step = 0;
    switch (s_cur_state) {
    case GAME_TITLE:
        if (gameTitle() == 0)
            s_next_state = GAME_START;
        break;
    case GAME_START:
        if (gameStart() == 0)
            s_next_state = GAME_PLAY;
        break;
    case GAME_PLAY:
        if (gamePlay() == 0)
            s_next_state = GAME_OVER;
        break;
    case GAME_OVER:
        rc = gameOver();
        s_next_state = (rc == 0) ? GAME_EXIT
                     : (rc == 1) ? GAME_OVER
                     : (rc == 2) ? GAME_START
                     :             GAME_TITLE;
        break;
    default: //case GAME_EXIT:
        return 0;
    }
    return 1;
}

/// タイトル.
/// @return  0:終了 1:継続.
static bool gameTitle(void) {
    cons_key_t   k        = cons_key();
    cons_clock_t cur_time = cons_clock();
    if (s_step < 255) {
        if (s_step < 6)
            k = CONS_KEY_ERR;
        ++s_step;
    }
    if (s_fall_time <= cur_time) { // 時間でピース変更.
        s_fall_time = cur_time + 12*GAME_MIN_SPEED;
        if (++s_piece_cur.shape > 6) {
            s_piece_cur.shape = 0;
            s_piece_cur.r     = (s_piece_cur.r + 1) & 3;
        }
    }
    rand();                     // 適当に乱数更新.
    s_draw_flags = DRAWF_ALL;
 #if defined(USE_SELECT_PIECE)
    if (k == 'C' || k == 'c') {
        select_piece_init(-2);
        k = CONS_KEY_ERR;
    }
 #endif
    return k == CONS_KEY_ERR;   // 何か入力されるのを待つ.
}

/// ゲーム開始.
/// @return  0:終了 1:継続.
static bool gameStart(void) {
    ++s_step;
    if (s_step == 1) {
        field_clear();
     #if defined(USE_SELECT_PIECE)
        select_piece_init(-1);
     #endif
        piece_init(&s_piece_cur);       // 最初のピースを用意.
        piece_init(&s_piece_next);      // 次のピースを用意.
        s_lines     = 0;
        s_pre_lines = 0;
        s_level     = 1;
        s_speed     = 10*GAME_MIN_SPEED;
        s_score     = 0;
    } else if (s_step > 13) {
        s_step      = 0;
        s_fall_time = cons_clock() + s_speed;
        return 0;
    }
    return 1;
}

/// ゲームプレイ.
/// @return  0:終了 1:継続.
static bool gamePlay(void) {
 #if !defined(MOTO_GAME)
    bool         clear_rq = 0;
 #endif
    cons_clock_t cur_time = cons_clock();
    uint8_t      k        = getKey();

    s_draw_flags = 0; //DRAWF_FIELD;

    if (s_piece_cur.y < 0) {
        s_draw_flags |= DRAWF_FIELD | DRAWF_INFO | DRAWF_NEXT;
    }

    // 入力処理.
    if (k) {
        Piece  cur = s_piece_cur;
        switch (k) {
        case key_left : --cur.x; break;
        case key_right: ++cur.x; break;
        case key_down : ++cur.y; break;
        case key_1    : cur.r = (cur.r + 1) & 3; break;
      #if !defined(MOTO_GAME)
        case key_2    : clear_rq = 1; break;
      #endif
        case key_cancel: return 0; // 強制終了.
        default: break;
        }
        if (field_canPlacePiece(&cur))
            s_piece_cur = cur;
        s_draw_flags |= DRAWF_FIELD;
    }

 #if !defined(MOTO_GAME)
    if (clear_rq) { // タメてた行をクリア.
        pieceClear();
        s_draw_flags |= DRAWF_FIELD | DRAWF_INFO;
    }
    if (s_lines < s_pre_lines) {
        s_draw_flags |= DRAWF_FIELD;
    }
 #endif

    // 落下.
    if (s_fall_time <= cur_time) {
        Piece   cur = s_piece_cur;
        s_fall_time = cur_time + s_speed;
        s_draw_flags |= DRAWF_FIELD;
        ++cur.y;
        if (field_canPlacePiece(&cur)) {    // 落下できる?
            ++s_piece_cur.y;
        } else {    // 着地.
            s_draw_flags |= DRAWF_INFO;
            field_placePiece(&s_piece_cur);
         #if !defined(MOTO_GAME)
            pieceLand();
         #else
            pieceLandMoto();
         #endif
            s_piece_cur = s_piece_next; // 次のピースをカレントに.
            piece_init(&s_piece_next);  // 新しい次のピースを用意.
            if (!field_canPlacePiece(&s_piece_cur))
                return 0;   // 出現場所で衝突 → GAME OVER.
        }
    }
    return 1;
}

#if !defined(MOTO_GAME)

/// 着地処理.
///
static void pieceLand(void) {
    uint8_t reached = filed_checkReach();
    if (reached > 0) {
        s_pre_lines += reached;
        s_score += reached * reached;
        if (s_high_score < s_score)
            s_high_score = s_score;
    }
}

/// ピース・クリア.
///
static void pieceClear(void) {
    uint8_t cleared = filed_clearLines();
    if (cleared > 0) {
        uint8_t i;
        s_lines += cleared;
        for (i = 1; i <= cleared; ++i)
            s_score += i;
        if (s_high_score < s_score)
            s_high_score = s_score;
        checkLevelUp();         // レベルアップ.
    }
}

#else // defined(MOTO_GAME)

/// 元ゲー:着地処理.
///
static void pieceLandMoto(void) {
    uint8_t cleared = filed_clearLinesMoto();
    if (cleared > 0) {
        s_lines  += cleared;
        s_score  += cleared*cleared;
        if (s_high_score < s_score)
            s_high_score = s_score;
        checkLevelUp();         // レベルアップ.
    }
}

#endif

/// レベルアップ・チェック.
///
static void checkLevelUp(void) {
    uint_t  new_level = s_lines / 10 + 1;
    if (s_level < new_level) {
        s_level   = new_level;
        s_speed   = (s_speed > 2*GAME_MIN_SPEED)
                  ? (s_speed - GAME_MIN_SPEED)
                  : GAME_MIN_SPEED;
    }
}

/// ゲームオーバー.
/// @return 1=処理中 2=リトライ 3=title 0=終了.
static uint8_t gameOver(void) {
    if (s_step == 0) {
        s_choise = 0;
        ++s_step;
    } else {
        uint8_t k = getKey();
        s_choise  = (s_choise + 3 - (k == key_left) + (k == key_right)) % 3;
        if (k == key_1 || k == key_2) {
            static uint8_t const rets[3] = { 2, 3, 0 };
            return rets[s_choise];
        } else if (k == key_cancel) {
            return 0;   // ESC は強制終了.
        }
    }
    return 1;   // 入力待ち.
}


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
//  GAME 画面データ.

#if CONS_COL_LIGHT == 0 // color 8

#define COL_DEFAULT                 (7)
#define COL_WALL                    (7)
#define COL_TITLE                   (4)
#define COL_START                   (6)
#define COL_GAMEOVER                (3)
#define COL_SUB                     (1)
#define COL_L_SUB                   (6)
#define COL_HELP                    (5)
#define COL_L_HELP                  (6)
#define COL_INP                     (5)
#define COL_L_INP                   (7)

#else                   // color 16

#define COL_DEFAULT                 (7)
#define COL_WALL                    (7)
#define COL_TITLE                   (4|CONS_COL_LIGHT)
#define COL_START                   (6|CONS_COL_LIGHT)
#define COL_GAMEOVER                (3|CONS_COL_LIGHT)
#define COL_SUB                     (6)
#define COL_L_SUB                   (6|CONS_COL_LIGHT)
#define COL_HELP                    (5)
#define COL_L_HELP                  (5|CONS_COL_LIGHT)
#define COL_INP                     (7)
#define COL_L_INP                   (7|CONS_COL_LIGHT)

#endif  // CONS_COL_LIGHT


#if !defined(USE_SELECT_PIECE)

#if defined(__PCAT__) && (CONSINIT_FLAGS & 1)  // PC-AT 40x25

#define STR_SPC                     " "
#define STR_P_FIX                   " "
#define ATR_P_FIX                   CONS_COL_REVERSE
#define STR_P_FALL                  " "
#define ATR_P_FALL                  (CONS_COL_REVERSE|CONS_COL_LIGHT)
#define STR_P_REACH                 " "
#define ATR_P_REACH                 (CONS_COL_REVERSE|CONS_COL_LIGHT)
#define STR_WALL                    "|"
#define FIELD_SCALE_X(x)            (x)

#elif defined(__PC98__)             // PC98 80x25

#define STR_SPC                     "  "
#define STR_P_FIX                   "\x81\xA1"   // "■"
#define ATR_P_FIX                   0x00
#define STR_P_FALL                  "  "
#define ATR_P_FALL                  CONS_COL_REVERSE
#define STR_P_REACH                 "  "
#define ATR_P_REACH                 CONS_COL_REVERSE
#define STR_WALL                    "\x86\xA5"   // "┃"
#define FIELD_SCALE_X(x)            ((x) << 1)

#elif CONS_COL_REVERSE > 0          // 反転有.

#if CONS_COL_LIGHT > 0              // 16 色.
#define STR_SPC                     "  "
#define STR_P_FIX                   "  "
#define ATR_P_FIX                   CONS_COL_REVERSE
#define STR_P_FALL                  "  "
#define ATR_P_FALL                  (CONS_COL_REVERSE|CONS_COL_LIGHT)
#define STR_P_REACH                 "  "
#define ATR_P_REACH                 (CONS_COL_REVERSE|CONS_COL_LIGHT)
#define STR_WALL                    "||"
#define FIELD_SCALE_X(x)            ((x) << 1)
#else                               // 8色.
#define STR_SPC                     "  "
#define STR_P_FIX                   "[]"
#define ATR_P_FIX                   0x00
#define STR_P_FALL                  "  "
#define ATR_P_FALL                  CONS_COL_REVERSE
#define STR_P_REACH                 "  "
#define ATR_P_REACH                 CONS_COL_REVERSE
#define STR_WALL                    "||"
#define FIELD_SCALE_X(x)            ((x) << 1)
#endif

#else                               // 反転無.

#define STR_SPC                     " "
#define STR_P_FIX                   "O"
#define ATR_P_FIX                   0x00
#define STR_P_FALL                  "O"
#define ATR_P_FALL                  0x00
#define STR_P_REACH                 "#"
#define ATR_P_REACH                 CONS_COL_LIGHT
#define STR_WALL                    "|"
#define FIELD_SCALE_X(x)            (x)

#endif

#else   // USE_SELECT_PIECE ピース・スタイル選択可能.(デバッグ向き)

#define STR_SPC                     (s_piece_parts->part[0])
#define STR_P_FIX                   (s_piece_parts->part[1])
#define ATR_P_FIX                   (s_piece_parts->colofs[1-1])
#define STR_P_FALL                  (s_piece_parts->part[2])
#define ATR_P_FALL                  (s_piece_parts->colofs[2-1])
#define STR_P_REACH                 (s_piece_parts->part[3])
#define ATR_P_REACH                 (s_piece_parts->colofs[3-1])
#define STR_WALL                    (s_piece_parts->part[4])
#define FIELD_SCALE_X(x)            ((x) << s_field_shift_x)

typedef struct PieceParts {
    uint8_t     shift_x;    ///< フィールド座標->コンソール座標 変換シフト数.
    uint8_t     colofs[3];  ///< 色オフセット.
    char const* part[5];    ///< 欠片 [0]空白 [1]固定 [2]落下 [3]リーチ [4]壁.
} PieceParts;

static PieceParts  const piece_parts_tbl[] = {
    { 1, { 0x10, 0x18, 0x18 }, { "  ", "  ", "  ", "  ", "||" } },
    { 0, { 0x10, 0x18, 0x18 }, { " " , " " , " " , " " , "|"  } },
    { 0, {    0,    8,    8 }, { " " , "O" , "O" , "#" , "|"  } },
    { 1, {    0,    8,    8 }, { "  ", "[]", "[]", "[]", "||" } },
 #if defined(CONS_USE_UNICODE)
    { 1, {    0,    8, 0x18 }, { "  ", "■", "■", "  ", "‖" } },
 #endif
};
enum { PiecePartsTbl_size = sizeof(piece_parts_tbl) / sizeof(piece_parts_tbl[0]) };

static PieceParts const*    s_piece_parts   = &piece_parts_tbl[0];
static uint8_t              s_field_shift_x = 0;
static uint8_t              s_piece_stype   = 0;    ///< ピースの表示スタイル.

/// ピース・スタイル設定.
///
static void select_piece_init(int piece_stype) {
    if (piece_stype == -1)
        piece_stype = s_piece_stype;
    else if (piece_stype == -2)
        piece_stype = s_piece_stype + 1;
    piece_stype     = piece_stype % PiecePartsTbl_size;
    s_piece_stype   = piece_stype;
    s_piece_parts   = &piece_parts_tbl[piece_stype];
    s_field_shift_x = s_piece_parts->shift_x;
}
#endif  // USE_SELECT_PIECE

// 変数.
static pos_t    s_draw_field_x;
static pos_t    s_draw_field_y;


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
//  GAME 描画.

#define PIECE_SHAPE_TO_COLOR(co)    ((co) + 1)

static void     draw_gameTitle(void);
static void     draw_gameStart(void);
static void     draw_gamePlay(void);
static void     draw_gameOver(void);

/// 毎フレームの描画更新.
///
static void draw_gameUpdate(void) {
    if (s_prev_state != s_cur_state || s_draw_flags == DRAWF_ALL) {
        // cons:テキスト画面バッファ・クリア.
        cons_clear();
        s_draw_flags = DRAWF_ALL;
    } else {
        cons_setRefreshRect(0, 0, 0, 0, 0);
    }
    switch (s_cur_state) {
    case GAME_TITLE: draw_gameTitle(); break;
    case GAME_START: draw_gameStart(); break;
    case GAME_PLAY:  draw_gamePlay();  break;
    case GAME_OVER:  draw_gameOver();  break;
    default: break;
    }
}

/// ピース描画.
///
static void draw_piece(pos_t x, pos_t y, uint8_t shape, uint8_t rot, bool bk) {
    uint16_t ptn  = piece_shapes[shape][rot];
    uint8_t  co   = PIECE_SHAPE_TO_COLOR(shape & 7);
    uint8_t  i;
    co += ATR_P_FALL;
    for (i = 0; i < 16; ++i) {
        pos_t x2 = x + FIELD_SCALE_X(i &  3);
        pos_t y2 = y + (i >> 2);
        if (y2 >= 0) {
            if (ptn & (0x8000 >> i)) {
                cons_xycputs(x2, y2, co, STR_P_FALL);
            } else if (bk) {
                cons_xycputs(x2, y2, COL_DEFAULT, STR_SPC);
            }
        }
    }
}

/// タイトル描画.
///
static void draw_gameTitle(void) {
    pos_t   w  = cons_screenWidth();
    pos_t   y  = (cons_screenHeight() - 18) >> 1;
    uint8_t co = (cons_tick() & 0x30) ? COL_L_INP : COL_INP;
 #if !defined(MOTO_GAME)
    char const* ttl = "O T I T A M E";
 #else
    char const* ttl = "O T I - G E";
 #endif
    cons_xycputs((w-strlen(ttl))>>1, y+2, COL_TITLE, ttl);
    cons_xycputs((w-11)>>1, y+14, co, "HIT ANY KEY");
 #if defined(USE_SELECT_PIECE)
    cons_xycputs((w-21)>>1, y+16, co, "([C]hange the pieces)");
    if (s_step == 1)
        select_piece_init(-1);
 #endif
    draw_piece((w-FIELD_SCALE_X(4))>>1,y+7,s_piece_cur.shape,s_piece_cur.r,1);
    if (s_step > 1)
        cons_setRefreshRect(0, (w-24)>>1, y+2, 24, 16-2);   // 画面更新範囲.
}

/// ゲーム開始描画.
///
static void draw_gameStart(void) {
    draw_gamePlay();
    if (s_step > 1) {
        pos_t x = s_draw_field_x + ((FIELD_SCALE_X(FIELD_W) - 10)>>1);
        pos_t y = (cons_screenHeight() - 1) >> 1;
        cons_xycputs(x, y, COL_START, "S T A R T!");
    }
}

/// ゲームプレイ画面表示.
///
static void draw_gamePlay(void) {
    uint8_t x, y;
    pos_t   ofs_x = (cons_screenWidth()  - FIELD_SCALE_X(FIELD_W)) >> 1;
    pos_t   ofs_y = (cons_screenHeight() - FIELD_H) >> 1;
 #if (CONSINIT_FLAGS & 1) == 1  // スクリーン横幅40文字の時.
    ofs_x -= 10;
 #endif

    if (ofs_x < 0) ofs_x = 0;
    if (ofs_y < 0) ofs_y = 0;

    s_draw_field_x = ofs_x;
    s_draw_field_y = ofs_y;

    // フィールド表示.
    if (s_draw_flags & DRAWF_FIELD) {
        pos_t dh = FIELD_H;
        if (ofs_y > 0) {    // フィールド1つ上のクリア.
            cons_xycprintf(ofs_x, ofs_y-1, COL_DEFAULT
                    , "%*c", FIELD_SCALE_X(FIELD_W), ' ');
            ++dh;
        }
        for (y = 0; y < FIELD_H; ++y) {
            pos_t y2 = ofs_y + y;
            for (x = 0; x < FIELD_W; ++x) {
                pos_t   x2  = ofs_x + FIELD_SCALE_X(x);
                field_t fld = field_get(x,y);
                if (fld) {
                    uint8_t shape = (fld & 7) - 1;
                    uint8_t co    = PIECE_SHAPE_TO_COLOR(shape);
                 #if !defined(MOTO_GAME)
                    if ((fld & 8) && (cons_tick() & 0x18))
                        cons_xycputs(x2, y2, co + ATR_P_REACH, STR_P_REACH);
                    else
                        cons_xycputs(x2, y2, co + ATR_P_FIX, STR_P_FIX);
                 #else
                    cons_xycputs(x2, y2, co + ATR_P_FALL, STR_P_FALL);
                 #endif
                } else {
                    cons_xycputs(x2, y2, COL_DEFAULT, STR_SPC);
                }
            }
        }
        //cons_xyprintf(ofs_x,ofs_y-1,"%6lx n:%6lx",cons_clock(),s_fall_time);

        // 現在のピースを表示.
        x = ofs_x + FIELD_SCALE_X(s_piece_cur.x);
        y = ofs_y + s_piece_cur.y;
        draw_piece(x, y, s_piece_cur.shape, s_piece_cur.r, 0);
        if (s_draw_flags != DRAWF_ALL)
            cons_setRefreshRect(0,ofs_x,ofs_y-1,FIELD_SCALE_X(FIELD_W),dh);
    }

    // 次のピースを表示.
    if (s_draw_flags & DRAWF_NEXT) {
        x = ofs_x + FIELD_SCALE_X(FIELD_W) + 4;
        y = ofs_y + 7;
        draw_piece(x, y, s_piece_next.shape, s_piece_next.r, 1);

        cons_setRefreshRect(1,x,y,FIELD_SCALE_X(4),4);
    }

    // 情報表示.
    if (s_draw_flags & DRAWF_INFO) {
        cons_setcolor(COL_DEFAULT);
        x  = ofs_x + FIELD_SCALE_X(FIELD_W) + 9;
        y  = ofs_y;
        cons_xyprintf(x, y+0, "%u", s_level);
        cons_xyprintf(x, y+1, "%u", s_lines);
      #if !defined(MOTO_GAME)
        if (s_pre_lines > s_lines) {
            cons_setcolor(COL_HELP);
            cons_printf(" (+%u)", s_pre_lines - s_lines);
            cons_setcolor(COL_DEFAULT);
        } else {
            cons_puts("          ");
        }
      #endif
        cons_xyprintf(x, y+2, "%u%s", s_score, s_score ? "00" : "");
        cons_xyprintf(x, y+3, "%u%s", s_high_score, s_high_score ? "00" : "");

        cons_setRefreshRect(2,x,y,14,4);
    }

    // ヘルプ: ENTER KEY.
  #if !defined(MOTO_GAME)
    if ((s_draw_flags & DRAWF_FIELD)) {
        uint8_t co = COL_HELP;
        if (s_pre_lines > s_lines && (cons_tick() & 0x18))
            co = COL_L_HELP;
        x  = ofs_x + FIELD_SCALE_X(FIELD_W+1) + 1;
        y  = ofs_y + 16;
        cons_xycputs(x,y+2,co,"Clear : ENTER  KEY");
        cons_setRefreshRect(3,x,y+2,20,1);
    }
  #endif

    // 固定表示物.
    if (s_draw_flags & DRAWF_TEXT) {
        char const* wall = STR_WALL;
        // 壁表示.
        cons_setcolor(COL_WALL);
        for (y = 0; y < FIELD_H; ++y) {
            cons_xyputs(ofs_x-FIELD_SCALE_X(1),       ofs_y+y, wall);
            cons_xyputs(ofs_x+FIELD_SCALE_X(FIELD_W), ofs_y+y, wall);
        }
        // 情報項目.
        cons_setcolor(COL_DEFAULT);
        x  = ofs_x + FIELD_SCALE_X(FIELD_W+1) + 1;
        y  = ofs_y;
        cons_xyputs(x, y+0, "Level");
        cons_xyputs(x, y+1, "Lines");
        cons_xyputs(x, y+2, "Score");
        cons_xyputs(x, y+3, "Hi-SC");
        cons_xyputs(x, y+5, " Next");

        // ヘルプ.
        cons_setcolor(COL_HELP);
        x  = ofs_x + FIELD_SCALE_X(FIELD_W+1) + 1;
      #if !defined(MOTO_GAME)
        y  = ofs_y + 16;
        cons_xyputs(x, y+0, "Move  : CURSOR KEY");
        cons_xyputs(x, y+1, "Rotate: SPACE  KEY");
        cons_xyputs(x, y+3, "Quit  : ESC    KEY");
      #else
        y  = ofs_y + 17;
        cons_xyputs(x, y+0, "Move  : CURSOR KEY");
        cons_xyputs(x, y+1, "Rotate: SPACE  KEY");
        cons_xyputs(x, y+2, "Quit  : ESC    KEY");
      #endif

    }
}

/// ゲームオーバー画面表示.
///
static void draw_gameOver(void) {
    pos_t   sc_w = cons_screenWidth();
    pos_t   sc_h = cons_screenHeight();
    pos_t   w    = 20;
    pos_t   h    = 10;
    pos_t   x    = (sc_w - w) >> 1;
    pos_t   y    = (sc_h - h) >> 1;

    if (s_draw_flags & DRAWF_OVER) {    // 実質初回のみ描画.
        char    buf[128];
        pos_t   l;
        draw_gamePlay();
        for (l = 0; l < h; ++l) // 矩形でなく帯で描画.
            cons_xycprintf(0, y + l, COL_DEFAULT, "%*c", sc_w-1, ' ');
        cons_xycputs(x+((w-16)>>1), y+2, COL_GAMEOVER , "G A M E  O V E R");
        snprintf(buf, sizeof(buf), "Score: %u%s", s_score, s_score ? "00":"");
        cons_xycputs(x+((w-strlen(buf))>>1), y+5, COL_L_SUB , buf);
        //cons_setRefreshRect(3,x,y,w,h);
    } else {
        cons_setRefreshRect(3,x,y+7,w,1);   // 選択肢のみ更新.
    }
    {   //選択肢表示.
        static char const sels[2][3][8] = {
            { " Retry ", " Title ", " Exit ", },
            { "[Retry]", "[Title]", "[Exit]", },
        };
        static uint8_t const col[2] = { COL_INP, COL_L_INP, };
        uint8_t n;
        y += 7;
        n = (s_choise == 0);
        cons_xycputs(x      , y, col[n], sels[n][0]);
        n = (s_choise == 1);
        cons_xycputs(x +   7, y, col[n], sels[n][1]);
        n = (s_choise == 2);
        cons_xycputs(x + 2*7, y, col[n], sels[n][2]);
    }
}


// -    -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/// オプション取得.
///
static void getOpt(char const* a) {
    if (strncmp(a, "-piece", 6) == 0) {
     #if defined(USE_SELECT_PIECE)
        select_piece_init((uint8_t)atoi(a+6));
     #endif
    } else if (strncmp(a, "-hs", 3) == 0) {
        s_high_score = atoi(a+3);
    }
}

#define CFG_NAME    "otitame.cfg"

/// ファイルからオプション取得.
///
static void loadOpts(void) {
    char  buf[128];
    FILE* fp = fopen(CFG_NAME, "rt");
    if (fp) {
        while (fgets(buf, sizeof buf, fp) != NULL)
            getOpt(buf);
        fclose(fp);
    }
}

/// ファイルへオプション保存.
///
static void saveOpts(void) {
    FILE* fp = fopen(CFG_NAME, "wt");
    if (fp) {
        fprintf(fp, "-hs%d\n"   , s_high_score);
     #if defined(USE_SELECT_PIECE)
        fprintf(fp, "-piece%d\n", s_piece_stype);
     #endif
        fclose(fp);
    }
}

/// Main
///
int main(int argc, char* argv[]) {
    int i;
    loadOpts();
    for (i = 1; i < argc; ++i)
        getOpt(argv[i]);
    gameMain();
    saveOpts();
    return 0;
}
