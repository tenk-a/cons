#include "cons.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
//  etc

typedef unsigned int    uint_t;
typedef cons_pos_t      pos_t;

#define FIELD_W         10
#define FIELD_H         20

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

static Piece    s_piece_cur;    ///< 現在のピース.
static Piece    s_piece_next;   ///< 次のピース.

/// ピース初期化.
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
static void field_clear(void) {
    memset(s_field, 0, sizeof(s_field));
}

/// ピースを置けるか?
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

/// ライン消去.
static uint8_t filed_clearLines(void) {
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

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
//  GAME

#define GAME_MIN_SPEED   CONS_MSEC_TO_CLOCK(50) ///< 最小速度(ミリ秒)

static cons_clock_t s_fall_time   = 0;      ///< 次の落下予定時間.
static uint_t       s_lines       = 0;      ///< クリアしたライン数.
static uint_t       s_level       = 1;      ///< レベル.
static uint_t       s_score       = 0;      ///< スコア.
static uint_t       s_high_score  = 0;      ///< ハイスコア.
static uint_t       s_speed       = 0;      ///< 落下速度.
static uint8_t      s_step        = 0;      ///< そのステートでのstep.

/// タイトル.
/// @return  0:終了 1:継続.
static bool gameTitle(void) {
    cons_key_t  k = cons_key();
    if (s_fall_time <= cons_clock()) { // 時間でピース変更.
        s_fall_time = s_fall_time + 12 * GAME_MIN_SPEED;
        if (++s_piece_cur.shape > 6) {
            s_piece_cur.shape = 0;
            s_piece_cur.r     = (s_piece_cur.r + 1) & 3;
        }
    }
    rand();                     // 適当に乱数更新.
    s_step = 0;
    return k == CONS_KEY_ERR;   // 何か入力されるのを待つ.
}

/// ゲーム開始.
/// @return  0:終了 1:継続.
static bool gameStart(void) {
    ++s_step;
    if (s_step == 1) {
        field_clear();
        piece_init(&s_piece_cur);       // 最初のピースを用意.
        piece_init(&s_piece_next);      // 次のピースを用意.
        s_lines     = 0;
        s_level     = 1;
        s_speed     = 11 * GAME_MIN_SPEED;
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
    cons_clock_t cur_time = cons_clock();
    cons_key_t   k        = cons_key();
    // 入力処理.
    if (k != CONS_KEY_ERR) {
        Piece    cur      = s_piece_cur;
        switch (k) {
        case CONS_KEY_LEFT:
            --cur.x;
            break;
        case CONS_KEY_RIGHT:
            ++cur.x;
            break;
        case CONS_KEY_DOWN:
            ++cur.y;
            break;
        case ' ':
            cur.r = (cur.r + 1) & 3;
            break;
        case CONS_KEY_ESC:
            return 0; // 強制終了.
        default:
            break;
        }
        if (field_canPlacePiece(&cur))
            s_piece_cur = cur;
    }

    // 落下.
    if (s_fall_time <= cur_time) {
        Piece   cur = s_piece_cur;
        s_fall_time = cur_time + s_speed;
        ++cur.y;
        if (field_canPlacePiece(&cur)) {    // 落下できる?
            ++s_piece_cur.y;
        } else {    // 着地.
            uint_t  new_level;
            uint8_t i, cleared;
            field_placePiece(&s_piece_cur);
            cleared = filed_clearLines();
            if (cleared > 0) {
                s_lines  += cleared;
                for (i = 1; i <= cleared; ++i)
                    s_score += i;
                // レベルアップ.
                new_level = s_lines / 10 + 1;
                if (s_level < new_level) {
                    s_level = new_level;
                    s_speed = (s_speed > 2 * GAME_MIN_SPEED)
                              ? (s_speed - GAME_MIN_SPEED)
                              : GAME_MIN_SPEED;
                }
            }
            s_piece_cur = s_piece_next; // 次のピースをカレントに.
            piece_init(&s_piece_next);  // 新しい次のピースを用意.
            if (!field_canPlacePiece(&s_piece_cur))
                return 0;   // フィールド天井で衝突 → GAME OVER.
        }
    }
    return 1;
}

/// ゲームオーバー.
/// @return 1=処理中 2=リトライ 0=終了.
static uint8_t gameOver(void) {
    cons_key_t k = cons_key();
    if (s_high_score < s_score)
        s_high_score = s_score;
    s_step = 0;
    if (k == 'r' || k == 'R')
        return 2;   // リトライ.
    else if (k == 'q' || k == 'Q')
        return 0;   // 終了.
    return 1;       // 入力待ち.
}

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
//  GAME 描画.

#define COL_DEFAULT                 (7)
#define COL_L_DEFAULT               (7|CONS_COL_LIGHT)
#define COL_SUB                     (5|CONS_COL_LIGHT)
#define COL_TITLE                   (4|CONS_COL_LIGHT)
#define COL_START                   (6|CONS_COL_LIGHT)
#define COL_GAMEOVER                (3|CONS_COL_LIGHT)
#define COL_HELP                    (5)
#define COL_WALL                    (7)

#define PIECE_SHAPE_TO_COLOR(co)    ((co) + 1)
#define FIELD_SCALE_X(x)            ((x)  * 2)

/// ピースの１キャラ描画.
static void draw_shapeCh(pos_t x, pos_t y, field_t fld, uint8_t type) {
    uint8_t co;
    if (fld) {
        uint8_t  shape = fld - 1;
        co  = PIECE_SHAPE_TO_COLOR(shape) | CONS_COL_REV | CONS_COL_LIGHT;
    } else {
        co   = COL_DEFAULT;
    }
    cons_xycprintf(x, y, co, "  ");
}

/// ピース描画.
static void draw_piece(pos_t x, pos_t y, uint8_t shape, uint8_t rot) {
    uint16_t ptn  = piece_shapes[shape][rot];
    uint8_t  i;
    for (i = 0; i < 16; ++i) {
        if (ptn & (0x8000 >> i)) {
            pos_t x2 = x + FIELD_SCALE_X(i & 3);
            pos_t y2 = y + (i >> 2);
            if (y2 >= 0)
                draw_shapeCh(x2, y2, shape+1, 2);
        }
    }
}

/// タイトル描画.
static void draw_gameTitle(void) {
    pos_t   w  = cons_screenWidth();
    pos_t   y  = (cons_screenHeight() - 18) >> 1;
    uint8_t co = (cons_clock() & 0x30) ? COL_L_DEFAULT : COL_DEFAULT;
    cons_xycprintf((w-11)>>1, y+2, COL_TITLE, "O T I - G E");
    draw_piece((w-FIELD_SCALE_X(4))>>1, y+7, s_piece_cur.shape, s_piece_cur.r);
    cons_xycprintf((w-11)>>1, y+14, co, "HIT ANY KEY");
}

static void draw_gamePlay(void);

/// ゲーム開始描画.
static void draw_gameStart(void) {
    pos_t x, y;
    draw_gamePlay();
    x   = (cons_screenWidth() - 10) >> 1;
    y   = (cons_screenHeight() - 1) >> 1;
    if (s_step > 1)
        cons_xycprintf(x, y, COL_START, "S T A R T!");
}

/// ゲームプレイ画面表示.
static void draw_gamePlay(void) {
    uint8_t x, y, co;
    pos_t   ofs_x = (cons_screenWidth()  - FIELD_SCALE_X(FIELD_W)) >> 1;
    pos_t   ofs_y = (cons_screenHeight() - FIELD_H) >> 1;

    if (ofs_x < 0) ofs_x = 0;
    if (ofs_y < 0) ofs_y = 0;

    // フィールド表示.
    for (y = 0; y < FIELD_H; ++y) {
        cons_xycprintf(ofs_x-FIELD_SCALE_X(1), ofs_y+y, COL_WALL, "||");
        for (x = 0; x < FIELD_W; ++x) {
            field_t fld = field_get(x,y);
            draw_shapeCh(ofs_x+FIELD_SCALE_X(x), ofs_y+y, fld, 1);
        }
        cons_xycprintf(ofs_x+FIELD_SCALE_X(FIELD_W), ofs_y+y, COL_WALL, "||");
    }

    // 現在のブロックを表示.
    x = ofs_x + FIELD_SCALE_X(s_piece_cur.x);
    y = ofs_y + s_piece_cur.y;
    draw_piece(x, y, s_piece_cur.shape, s_piece_cur.r);

    // 情報表示.
    co = COL_DEFAULT;
    x  = ofs_x + FIELD_SCALE_X(FIELD_W) + 3;
    y  = ofs_y;
    cons_xycprintf(x, y+0, co, "Score: %u%s", s_score, s_score ? "00" : "");
    cons_xycprintf(x, y+1, co, "Level: %u"  , s_level);
    cons_xycprintf(x, y+2, co, "Lines: %u"  , s_lines);
    cons_xycprintf(x, y+3, co, "Hi-SC: %u%s", s_high_score, s_high_score ? "00" : "");

    // 次のピースを表示.
    x  = ofs_x + FIELD_SCALE_X(FIELD_W) + 4;
    cons_xycprintf(x, y + 5, co, "Next:");
    y  = ofs_y + 7;
    draw_piece(x, y, s_piece_next.shape, s_piece_next.r);

    // キー説明.
    co = COL_HELP;
    x  = ofs_x + FIELD_SCALE_X(FIELD_W) + 3;
    y  = ofs_y + 17;
    cons_xycprintf(x, y+0, co, "Move   : CURSOR KEY");
    cons_xycprintf(x, y+1, co, "Rotate : SPACE  KEY");
    cons_xycprintf(x, y+2, co, "Quit   : ESC    KEY");
}

/// ゲームオーバー画面表示.
static void draw_gameOver(void) {
    pos_t   sc_w = cons_screenWidth();
    pos_t   sc_h = cons_screenHeight();
    pos_t   w  = 20, h = 10;
    pos_t   x  = (sc_w - w) >> 1;
    pos_t   y  = (sc_h - h) >> 1;
    uint8_t co = (cons_clock() & 0x30) ? COL_L_DEFAULT : COL_DEFAULT;
    uint8_t i;

    draw_gamePlay();
    for (i = 0; i < h; ++i)
        cons_xycprintf(0, y + i, COL_DEFAULT, "%*c", sc_w-1, ' ');
    cons_xycprintf(x+((w-16)>>1), y+2, COL_GAMEOVER , "G A M E  O V E R");
    cons_xycprintf(x+((w-16)>>1), y+7, co           , "[R]etry / [Q]uit");
}

// -    -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/// Main
int main(void) {
    enum { TITLE=1, START, PLAY, OVER };
    uint8_t state = TITLE;
    srand((int)time(NULL));                     // 乱数初期化.
    if (cons_init() == 0)                       // cons:コンソール画面初期化.
        return 1;
    // ゲーム・ループ.
    for (;;) {
        cons_updateBegin();                     // cons:毎フレームの開始処理.
        // state 毎の処理.
        if (state == TITLE) {
            state = gameTitle()? TITLE : START; // タイトル画面.
        } else if (state == START) {
            state = gameStart()? START : PLAY;  // ゲーム開始.
        } else if (state == PLAY) {
            state = gamePlay() ? PLAY : OVER;   // ゲーム・プレイ.
        } else if (state == OVER) {
            uint8_t rc = gameOver();            // ゲームオーバー画面.
            if (!rc)                            // 終了.
                break;
            state = (rc == 1) ? OVER : START;   // rc=2 で再プレイ.
        }
        // state 毎の表示.
        cons_clear();
        switch (state) {
        case TITLE: draw_gameTitle(); break;
        case START: draw_gameStart(); break;
        case PLAY : draw_gamePlay();  break;
        case OVER : draw_gameOver();  break;
        }
        cons_updateEnd();                       // cons:毎フレーム終わり処理.
    }
    cons_term();                                // cons:コンソール終了処理.
    return 0;
}
