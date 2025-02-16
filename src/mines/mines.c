/**
 * @file mines.c
 * @brief TUIマインスイーパー
 * @author Masashi Kitamura ( https://github.com/tenk-a/ )
 * @date   2024-12
 * @license Boost Software License - Version 1.0
 * @note
 *   pdcurses/ncurses、pc-at dos, pc98 dos 用.
 */
#include "cons.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#if __STDC_VERSION__ >= 199901L || __cplusplus >= 201103L
 #include <stdint.h>
 #if !defined(__cplusplus)
  #include <stdbool.h>
 #endif
#else
typedef signed   char   int8_t;
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned char   bool;
#endif


//-----------------------------------------------------------------------------

typedef cons_pos_t      pos_t;
typedef unsigned int    uint_t;

enum { GAME_EXIT, GAME_TITLE, GAME_START, GAME_PLAY, GAME_WIN, GAME_OVER };
enum { key_up=1, key_down, key_left, key_right, key_1, key_2, key_cancel };

#define MINE_MAP_MAX_W     30       ///< マップ最大横幅.
#define MINE_MAP_MAX_H     16       ///< マップ最大縦幅.

#define MINE_CELL_MASK     0x0F     ///< 0=空, 1..8=隣接爆弾数, 9=爆弾.
#define MINE_CELL_CLOSE    0x10     ///< クローズ(未オープン)フラグ.
#define MINE_CELL_FLAG     0x20     ///< フラグ ON

#define MINE_CELL_BOMB     9        ///< 爆弾.
#define MINE_CELL_EMPTY    0        ///< 空き.

/// マップ・レベル情報.
typedef struct map_level_t {
    uint8_t w, h, bomb;
} map_level_t;
map_level_t const s_map_levels[3] = {
    {  9,  9, 10 },     // Small
    { 16, 16, 40 },     // Middle
    { 30, 16, 99 },     // Large
};

/// 周囲8方向オフセット.
static int8_t const s_dxy[8][2] = {
    {-1,-1}, {0,-1}, {1,-1},
    {-1, 0},         {1, 0},
    {-1,+1}, {0,+1}, {1,+1},
};

static uint8_t      s_map[MINE_MAP_MAX_H][MINE_MAP_MAX_W]; ///< マップ情報.

static pos_t        s_map_w;        ///< マップ横幅.
static pos_t        s_map_h;        ///< マップ縦幅.
static uint8_t      s_bomb_total;   ///< 爆弾の総数.
static uint8_t      s_map_level;    ///< 爆弾の総数.

static int8_t       s_flag_count;   ///< 現在マーク中のフラグ数.

static int8_t       s_cursor_x;     ///< カーソルの現在位置(x)
static int8_t       s_cursor_y;     ///< カーソルの現在位置(y)
static uint8_t      s_step;         ///< ステート内のステップ.
static uint8_t      s_choise;       ///< 選択.
static bool         s_play_draw_rq; ///< gamePlay での画面更新リクエスト.

static cons_clock_t s_cur_time;     ///< 現在時間.
static cons_clock_t s_start_time;   ///< ゲーム開始時刻.

/// セルをオープンする場所を貯めるキュー.
typedef struct open_cell_que_t {
    uint8_t x,y;
} open_cell_que_t;

/// 一時利用のテンポラリ・バッファ.
static char         s_static_buf[MINE_MAP_MAX_W * MINE_MAP_MAX_H * 2];

#if defined(__PCAT__)
#define CONSINIT_FLAGS      1   // text 40x25.
#else
#define CONSINIT_FLAGS      0   // デフォルト(text 80x25想定)
#endif


//-----------------------------------------------------------------------------
//  etc.

/// 乱数初期化.
///
static void rand_init(void) {
    srand((unsigned int)time(NULL));
}

/// 0～(n-1) を生成する乱数.
///
static uint8_t rand_n(uint8_t n) {
    uint_t d = RAND_MAX / n;
    uint_t m = d * n;
    uint_t i;
    do {
        i = rand();
    } while (i >= m);
    i = i / d;
    return i;
}

/// キー入力.
///
static uint8_t getKey(void) {
    switch (cons_key()) {
    case CONS_KEY_UP:   case 'w': case 'W':
        return key_up;
    case CONS_KEY_DOWN: case 's': case 'S':
        return key_down;
    case CONS_KEY_LEFT: case 'a': case 'A':
        return key_left;
    case CONS_KEY_RIGHT:case 'd': case 'D':
        return key_right;
    case CONS_KEY_SPACE:case 'z': case 'Z':
        return key_1;
    case CONS_KEY_RETURN:case 'x':case 'X':
        return key_2;
    case CONS_KEY_ESC:  case 'c': case 'C':
        return key_cancel;
    default:
        return 0;
    }
}

//-----------------------------------------------------------------------------
// マップ関係.

/// マップクリア.
///
static void mine_mapClear(void) {
    uint8_t x, y;
    for (y = 0; y < s_map_h; ++y) {
        for (x = 0; x < s_map_w; ++x) {
            s_map[y][x] = MINE_CELL_CLOSE;
        }
    }
}

/// セルの値. 0:空 1..8:爆弾隣接数. 9:爆弾. bit3:close中 bit4:フラグ.
///
static inline uint8_t mine_cellValue(uint8_t cell) {
    return (cell & MINE_CELL_MASK);
}

/// セルが閉じているか.
///
static inline bool mine_isClosed(uint8_t cell) {
    return (cell & MINE_CELL_CLOSE) != 0;
}

/// フラグが置かれているか?
///
static inline bool mine_isFlagged(uint8_t cell) {
    return (cell & MINE_CELL_FLAG) != 0;
}

/// 爆弾設置→隣接カウント更新.
///
static void mine_setupBombs(void) {
    uint8_t n = 0, y, x;
    while (n < s_bomb_total) {
        uint8_t rx = rand_n(s_map_w);
        uint8_t ry = rand_n(s_map_h);
        uint8_t v  = mine_cellValue(s_map[ry][rx]);
        if (v != MINE_CELL_BOMB) {
            // 爆弾にする(9) + 閉(16)
            s_map[ry][rx] = MINE_CELL_BOMB | MINE_CELL_CLOSE;
            ++n;
        }
    }

    // 隣接数を求める.
    for (y = 0; y < s_map_h; ++y) {
        for (x = 0; x < s_map_w; ++x) {
            uint8_t i;
            uint8_t bomb_ct = 0;
            uint8_t v = mine_cellValue(s_map[y][x]);
            if (v == MINE_CELL_BOMB)
                continue;
            // 周囲の爆弾数をカウント.
            for (i = 0; i < 8; ++i) {
                int8_t nx = x + s_dxy[i][0];
                int8_t ny = y + s_dxy[i][1];
                if (nx >= 0 && nx < s_map_w && ny >= 0 && ny < s_map_h) {
                    if (mine_cellValue(s_map[ny][nx]) == MINE_CELL_BOMB) {
                        ++bomb_ct;
                    }
                }
            }
            s_map[y][x] = (bomb_ct & 0x0F) | MINE_CELL_CLOSE;
        }
    }
}

/// 指定セルおよびその周辺のオープン.
///
static void mine_openCell(uint8_t x, uint8_t y) {

    uint8_t  cell = s_map[y][x];
    cell         &= ~(MINE_CELL_CLOSE | MINE_CELL_FLAG);
    s_map[y][x] = cell;

    // もし周囲爆弾数が0なら、周囲も自動で開く.
    if (mine_cellValue(cell) == 0) {
        open_cell_que_t* front = (open_cell_que_t*)(s_static_buf);
        open_cell_que_t* back  = front;
        uint8_t          i;
        back->x = x;
        back->y = y;
        ++back;
        while (front < back) {
            x = front->x;
            y = front->y;
            ++front;

            // 8方向を開く
            for (i = 0; i < 8; ++i) {
                int8_t x2 = x + s_dxy[i][0];
                int8_t y2 = y + s_dxy[i][1];
                if (x2 < 0 || x2 >= s_map_w || y2 < 0 || y2 >= s_map_h)
                    continue;
                cell = s_map[y2][x2];
                if (mine_isClosed(cell) && !mine_isFlagged(cell)) {
                    s_map[y2][x2] &= ~(MINE_CELL_CLOSE|MINE_CELL_FLAG);
                    if (mine_cellValue(cell) == 0) {
                        back->x = x2;
                        back->y = y2;
                        ++back;
                    }
                }
            }
        }
    }
}

/// 勝利判定(爆弾以外が全て開いているか?)
///
static bool mine_checkClear(void) {
    uint8_t x, y;
    for (y = 0; y < s_map_h; ++y) {
        for (x = 0; x < s_map_w; ++x) {
            uint8_t v = mine_cellValue(s_map[y][x]);
            if (v != MINE_CELL_BOMB) {
                if (mine_isClosed(s_map[y][x])) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

/// 入力処理: カーソル移動.
///
static void mine_moveCursor(int dx, int dy) {
    s_cursor_x += dx;
    s_cursor_y += dy;
    if (s_cursor_x < 0)
        s_cursor_x = 0;
    else if (s_cursor_x >= s_map_w)
        s_cursor_x = s_map_w - 1;

    if (s_cursor_y < 0)
        s_cursor_y = 0;
    else if (s_cursor_y >= s_map_h)
        s_cursor_y = s_map_h - 1;
}


//-----------------------------------------------------------------------------
// 各ステート処理.

static uint8_t  gameTitle(void);
static uint8_t  gameStart(void);
static uint8_t  gamePlay(void);
static uint8_t  gameWin(void);
static uint8_t  gameOver(void);
static void     draw_game(uint8_t state);

/// ゲーム・メイン処理.
///
int gameMain(void) {
    int state = GAME_EXIT;
    int next  = GAME_TITLE;

    if (!cons_init(CONSINIT_FLAGS))
        return 1;

    rand_init();

    // ゲームループ.
    do {
        uint8_t rc;
        cons_updateBegin();

        // ステート遷移.
        if (state != next)
            s_step = 0;
        state = next;
        switch (state) {
        case GAME_TITLE:    // タイトル.
            rc   = gameTitle();
            next = (rc == 1) ? GAME_TITLE
                 : (rc == 0) ? GAME_EXIT
                 :             GAME_START;
            break;
        case GAME_START:    // ゲーム開始.
            next = gameStart() ? GAME_START : GAME_PLAY;
            break;
        case GAME_PLAY:     // ゲーム中.
            rc   = gamePlay();
            next = (rc == 1) ? GAME_PLAY
                 : (rc == 0) ? GAME_OVER
                 :             GAME_WIN;
            break;
        case GAME_WIN:      // 勝利.
            if (gameWin() == 0)
                next = GAME_TITLE;
            break;
        case GAME_OVER:     // ゲームオーバー.
            rc   = gameOver();
            next = (rc == 1) ? GAME_OVER
                 : (rc == 0) ? GAME_EXIT
                 : (rc == 2) ? GAME_START
                 :             GAME_TITLE;
            break;
        default:
            break;
        }

        // 描画.
        draw_game(state);

        cons_updateEnd();
    } while (next != GAME_EXIT);

    cons_term();
    return 0;
}

/// タイトル画面(モード選択)
///
static uint8_t gameTitle(void) {
    uint8_t k = getKey();   // キー入力.
    if (s_step < 6) {
        ++s_step;
        k = 0;
    }
    if (k) {
        s_map_level += -(k == key_up) + (k == key_down);
        s_map_level &= 3;
        if (k == key_1 || k == key_2) {
            if (s_map_level < 3) {
                return 2;   // ゲーム開始.
            }
            return 0;   // 終了.
        } else if (k == key_cancel) {
            return 0;
        }
    }
    return 1;   // タイトル画面継続.
}

/// ゲーム開始ステート.
///
static uint8_t gameStart(void) {
    map_level_t const* lv = &s_map_levels[s_map_level];
    s_map_w      = lv->w;
    s_map_h      = lv->h;
    s_bomb_total = lv->bomb;
    s_flag_count = 0;

    s_cursor_x   = s_map_w >> 1;
    s_cursor_y   = s_map_h >> 1;

    mine_mapClear();
    mine_setupBombs();

    s_start_time = cons_clock();
    s_cur_time   = s_start_time;

    return 0; // PLAY.
}

/// セルをオープンする.
///
uint8_t gamePlay_open(void) {
    uint8_t cx   = s_cursor_x;
    uint8_t cy   = s_cursor_y;
    uint8_t cell = s_map[cy][cx];

    // フラグが立ってたら何もしない.
    if (!mine_isFlagged(cell)) {
        // 爆弾なら -> GAME OVER
        if (mine_cellValue(cell) == MINE_CELL_BOMB) {
            return 0;       // 爆発.
        } else if (mine_isClosed(cell)) {
            mine_openCell(cx, cy);
            if (mine_checkClear()) {    // 勝利判定.
                return 2;   // 勝利.
            }
        }
    }
    return 1;
}

/// フラグの付け外し.
///
void gamePlay_changeFlag(void) {
    uint8_t cx   = s_cursor_x;
    uint8_t cy   = s_cursor_y;
    uint8_t cell = s_map[cy][cx];
    if (mine_isClosed(cell)) {
        if (mine_isFlagged(cell)) {
            s_map[cy][cx] &= ~MINE_CELL_FLAG;
            --s_flag_count;
        } else {
            if (s_flag_count < s_bomb_total) {
                s_map[cy][cx] |= MINE_CELL_FLAG;
                ++s_flag_count;
            }
        }
    }
}

/// ゲームメイン(PLAY)
/// @return 0=OVER 1=継続 2=WIN
static uint8_t gamePlay(void) {
    uint8_t k  = getKey();
    s_cur_time = cons_clock();
    s_play_draw_rq = 1;
    switch(k) {
    case key_left:  mine_moveCursor(-1,0); break;
    case key_right: mine_moveCursor(+1,0); break;
    case key_up:    mine_moveCursor(0,-1); break;
    case key_down:  mine_moveCursor(0,+1); break;
    case key_1:     return gamePlay_open();
    case key_2:     gamePlay_changeFlag(); break;
    case key_cancel:return 0;   // 強制ゲームオーバー.
    default: s_play_draw_rq = 0; break;
    }
    return 1;   // 継続.
}

/// 勝利/オーバー演出が終わったときに呼ばれる関数.
///
static void game_enableKey_fromDraw(void) {
    s_step = 3;
}

/// 勝利ステート
/// @return 0=終了orタイトル,1=継続
static uint8_t gameWin(void) {
    if (s_step < 2) {
        ++s_step;
    } else if (s_step == 2) {
    } else {
        if (cons_key() != CONS_KEY_ERR) {
            return 0;   // 何か押されたらタイトルへ.
        }
    }
    return 1;   // 継続.
}

/// ゲームオーバーステート(爆弾全表示など)
/// @return 0=終了orタイトル,1=継続,2=リトライ,3=タイトル.
static uint8_t gameOver(void) {
    if (s_step == 0) {
        // 爆弾をすべてオープン表示.
        uint8_t x, y;
        for (y = 0; y < s_map_h; ++y) {
            for (x = 0; x < s_map_w; ++x) {
                if (mine_cellValue(s_map[y][x]) == MINE_CELL_BOMB) {
                    s_map[y][x] &= ~(MINE_CELL_CLOSE|MINE_CELL_FLAG);
                }
            }
        }
        s_choise = 0xff;
        ++s_step;
    } else if (s_step == 1) {
            ++s_step;
    } else if (s_step == 2) {
    } else {
        uint8_t k = getKey();
        s_choise = (s_choise + 3 - (k == key_left) + (k == key_right)) % 3u;
        if (k == key_1 || k == key_2) {
            static uint8_t const rets[3] = { 2, 3, 0 };
            return rets[s_choise];
        } else if (k == key_cancel) {
            return 0;   // ESC は強制終了.
        }
    }
    return 1;   // GAME OVER 継続.
}

//-----------------------------------------------------------------------------
// 描画素材.

#define COL_DEFAULT         (7 | CONS_COL_LIGHT)
#define COL_EMPTY           (7)
#define COL_CELL            (5 | CONS_COL_LIGHT)
#define COL_FLAG            (6 | CONS_COL_LIGHT)
#define COL_BOMB            (2 | CONS_COL_LIGHT)
#define COL_CELL_CUR        (7 | CONS_COL_LIGHT)
#define COL_NUMBER_1        (5 | CONS_COL_LIGHT)
#define COL_NUMBER_2        (4 | CONS_COL_LIGHT)
#define COL_NUMBER_3_TO_8   (3 | CONS_COL_LIGHT)
#define COL_TITLE           (5 | CONS_COL_LIGHT)
#define COL_TITLE_2         (1)
#define COL_CHOOSE          (6 | CONS_COL_LIGHT)
#define COL_GAMEOVER        (3 | CONS_COL_LIGHT)
#define COL_GAMEWIN         (6 | CONS_COL_LIGHT)
#define COL_CONG_FRAME      (7 | CONS_COL_LIGHT)
#define COL_CONG_1          (3 | CONS_COL_LIGHT)
#define COL_CONG_2          (6 | CONS_COL_LIGHT)
#define COL_WALL            (5)

#if defined(CONS_USE_UNICODE)
#define SCR_X_SHIFT         1
#define STR_EMPTY           "  "
#define STR_CELL_CUR        "◆"
#define STR_CELL            "■"
#define STR_FLAG            "▲"
#define STR_BOMB            "●"
#define STR_WALL_0          "┏"
#define STR_WALL_1          "━━"
#define STR_WALL_2          "━┓"
#define STR_WALL_3          "┃"
#define STR_WALL_4          "┃"
#define STR_WALL_5          "┗"
#define STR_WALL_6          "━━"
#define STR_WALL_7          "━┛"
#define STR_TITLE_CUR       "▶"
#define TITLE_CUR_W         2
#define STR_CONG_FRAME      "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
static char const str_digits[][4] = {"①","②","③","④","⑤","⑥","⑦","⑧"};

#elif defined(__PC98__) // NEC SJIS
#define SCR_X_SHIFT         1
#define STR_EMPTY           "  "
#define STR_CELL_CUR        "\x81\x9f"  // "◆"
#define STR_CELL            "\x81\xa1"  // "■"
#define STR_FLAG            "\x81\xa3"  // "▲"
#define STR_BOMB            "\x81\x9c"  // "●"
#define STR_WALL_0          "\x86\xB1"  // "┏"
#define STR_WALL_1          "\x86\xA3"  // "━"
#define STR_WALL_2          "\x86\xB5"  // "┓"
#define STR_WALL_3          "\x86\xA5"  // "┃"
#define STR_WALL_4          "\x86\xA5"  // "┃"
#define STR_WALL_5          "\x86\xB9"  // "┗"
#define STR_WALL_6          "\x86\xA3"  // "━"
#define STR_WALL_7          "\x86\xBD"  // "┛"
#define STR_TITLE_CUR       ">"         // ">"
#define TITLE_CUR_W         1
#define STR_CONG_FRAME      "\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3\x86\xA3"  // "━"
static char const str_digits[][4] = {
    "\x82\x50","\x82\x51","\x82\x52","\x82\x53",    // １２３４ .
    "\x82\x54","\x82\x55","\x82\x56","\x82\x57",    // ５６７８ .
};
#elif defined(__PCAT__) // CP437  40x24
#define SCR_X_SHIFT         0
#define STR_EMPTY           " "
#define STR_CELL_CUR        "\x04"  // "◆"
#define STR_CELL            "\xFE"  // "■"
#define STR_FLAG            "\x1E"  // "▲"
#define STR_BOMB            "\x0F"  // "●"
#define STR_WALL_0          "\xDA"  // "┏"
#define STR_WALL_1          "\xC4"  // "━"
#define STR_WALL_2          "\xBF"  // "┓"
#define STR_WALL_3          "\xB3"  // "┃"
#define STR_WALL_4          "\xB3"  // "┃"
#define STR_WALL_5          "\xC0"  // "┗"
#define STR_WALL_6          "\xC4"  // "━"
#define STR_WALL_7          "\xD9"  // "┛"
#define STR_TITLE_CUR       "\xAF"  // ">>"
#define TITLE_CUR_W         1
#define STR_CONG_FRAME      "\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4"
static char const str_digits[][3] = {"1","2","3","4","5","6","7","8"};

#else   // ASCII    // 80x24
#define SCR_X_SHIFT         1
#define STR_EMPTY           "  "
#define STR_CELL_CUR        "<>"
#define STR_CELL            "[]"
#define STR_FLAG            " F"
#define STR_BOMB            " *"
#define STR_WALL_0          " +"
#define STR_WALL_1          "--"
#define STR_WALL_2          "+ "
#define STR_WALL_3          " |"
#define STR_WALL_4          "| "
#define STR_WALL_5          " +"
#define STR_WALL_6          "--"
#define STR_WALL_7          "+ "
#define STR_TITLE_CUR       ">"
#define TITLE_CUR_W         1
#define STR_CONG_FRAME      "------------------------------------"
static char const str_digits[][3] = {" 1"," 2"," 3"," 4"," 5"," 6"," 7"," 8"};
#endif


//-----------------------------------------------------------------------------
// 描画系.

#define SCR_X_SCALE(x)  ((x) << SCR_X_SHIFT)
#define GAME_SCR_W      SCR_X_SCALE(MINE_MAP_MAX_W + 2)
#define GAME_SCR_H      (MINE_MAP_MAX_H + 4)

static cons_clock_t     s_draw_tick_0;
static pos_t            s_draw_map_ofs_x;
static pos_t            s_draw_map_ofs_y;
static uint8_t          s_draw_prev_state;
static uint8_t          s_draw_state;
static uint8_t          s_draw_map_level;

/// 文字列コピー. 最後のアドレスを返す.
///
static char* stpCpyE(char* dst, char* dst_e, char const* src) {
    size_t l = strlen(src);
    if (dst+l < dst_e) {
        memcpy(dst, src, l+1);
        return dst + l;
    } else if (dst < dst_e) {
        memset(dst, 0, dst_e - dst);
    }
    return dst_e;
}

/// ステータス表示.
///
static void draw_status(void) {
    int      x     = s_draw_map_ofs_x - 1;
    int      y     = s_draw_map_ofs_y - 2;
    unsigned w     = SCR_X_SCALE(s_map_w) + 2;
    if (w < 13) {
        x -= ((13 - w)>>1);
        w  = 13;
    }
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    if (s_play_draw_rq || s_draw_state != s_draw_prev_state) {
        uint_t   f_num = s_bomb_total - s_flag_count;
        cons_xycprintf(x, y, COL_FLAG, "%s", STR_FLAG);
        cons_xycprintf(x+3, y, COL_DEFAULT, "%2u", f_num);
        cons_setRefreshRect(1, x, y, w, 1);     // フラグ・時計範囲描画更新.
    } else {
        cons_setRefreshRect(1, x+w-5, y, 5, 1); // 時計のみの範囲を描画更新.
    }

    // 時計は毎フレーム更新(1秒単位でよいが判定を手抜き)
    {
        unsigned long diff  = (s_cur_time - s_start_time) / CONS_CLOCK_PER_SEC;
        unsigned      mm    = (unsigned)(diff / 60);
        unsigned      ss    = (unsigned)(diff % 60);
        if (mm > 99) {
            mm = 99;
            ss = 59;
        }
        cons_xycprintf(x + w - 5, y, COL_DEFAULT, "%2u:%02u", mm, ss);
    }
}

/// 外枠描画.
///
static void draw_frame(pos_t x, pos_t y, pos_t w, pos_t h) {
    enum { buf_sz = 6 * (MINE_MAP_MAX_W + 2) + 1 };
    char   buf[ buf_sz ];
    char*  d = buf;
    char*  e = d + buf_sz;
    pos_t  x2,y2;
    uint_t i;
    cons_col_t co = COL_WALL;
    if (s_draw_state == GAME_OVER)  //ゲームオーバー時は外枠の色を赤に.
        co = COL_BOMB;

    x -= SCR_X_SCALE(1);
    d = stpCpyE(d, e, STR_WALL_0);
    i = s_map_w;
    do {
        d = stpCpyE(d, e, STR_WALL_1);
    } while (--i);
    d = stpCpyE(d, e, STR_WALL_2);
    cons_xycputs(x, y-1, co, buf);

    d = buf;
    d = stpCpyE(d, e, STR_WALL_5);
    i = s_map_w;
    do {
        d = stpCpyE(d, e, STR_WALL_6);
    } while (--i);
    d = stpCpyE(d, e, STR_WALL_7);
    cons_xycputs(x, y+h, co, buf);

    // 左右.
    x2 = x + SCR_X_SCALE(w+1);
    for (y2 = y; y2 < y+h; ++y2) {
        cons_xycputs( x, y2, co, STR_WALL_3);
        cons_xycputs(x2, y2, co, STR_WALL_4);
    }
}

/// マップ表示.
///
static void draw_map(void) {
    pos_t   x, y;
    pos_t   map_w = SCR_X_SCALE(s_map_w);
    pos_t   ofs_x = (cons_screenWidth()  - map_w  ) >> 1;
    pos_t   ofs_y = (cons_screenHeight() - s_map_h) >> 1;
    if (ofs_x < 1)
        ofs_x = 1;
    if (ofs_y < 2)
        ofs_y = 2;
    s_draw_map_ofs_x = ofs_x;
    s_draw_map_ofs_y = ofs_y;

    draw_frame(ofs_x, ofs_y, s_map_w, s_map_h);

    for (y = 0; y < s_map_h; ++y) {
        uint8_t* line = &s_map[y][0];
        pos_t    y1   = ofs_y + y;
        pos_t    x1   = ofs_x;
        for (x = 0; x < s_map_w; ++x) {
            uint8_t cell = line[x];
            uint8_t val  = mine_cellValue(cell);
            if (mine_isClosed(cell)) {  // 閉じてる.
                if (mine_isFlagged(cell)) {
                    cons_xycputs(x1, y1, COL_FLAG, STR_FLAG);
                } else {
                    cons_xycputs(x1, y1, COL_CELL, STR_CELL);
                }
            } else {    // 開いてる.
                if (val >= MINE_CELL_BOMB) {
                    cons_xycputs(x1, y1, COL_BOMB, STR_BOMB);
                } else if (val == 0) {
                    cons_xycputs(x1, y1, COL_EMPTY, STR_EMPTY);
                } else {
                    uint8_t co;
                    if (val == 1)      co = COL_NUMBER_1;
                    else if (val == 2) co = COL_NUMBER_2;
                    else               co = COL_NUMBER_3_TO_8;
                    cons_xycputs(x1, y1, co, str_digits[val-1]);
                }
            }
            x1 += SCR_X_SCALE(1);
        }
    }
}

// カーソル表示.
///
void draw_cursor(void) {
    pos_t x = s_draw_map_ofs_x + SCR_X_SCALE(s_cursor_x);
    pos_t y = s_draw_map_ofs_y + s_cursor_y;
    cons_xycputs(x, y, COL_CELL_CUR, STR_CELL_CUR);
}


// -------------------------------------
// 各ステート描画.

/// タイトル画面表示.
///
static void draw_gameTitle(void) {
    int sw = cons_screenWidth();
    int sh = cons_screenHeight();
    int x  = (sw - 26)/2;
    int y  = (sh - 14)/2;
    uint8_t col[4] = { COL_DEFAULT, COL_DEFAULT, COL_DEFAULT, COL_DEFAULT };
    if (s_draw_state != s_draw_prev_state) {    // title初回描画.
        cons_xycputs(x+ 2, y+ 1, COL_TITLE  , "M I N E  S W E E P E R");
        cons_xycputs(x+ 3, y+ 2, COL_TITLE_2, "M I N E  S W E E P E R");
        s_draw_map_level = 0xff;    // 選択肢再描画のため前回位置を0-2以外に.
    }
    if (s_map_level != s_draw_map_level) {  // 選択が変わったら再描画.
        s_draw_map_level = s_map_level;
        col[s_map_level] = COL_CHOOSE;
        x += 5;
        cons_xycputs(x, y+ 7, col[0], "  SMALL  STAGE");
        cons_xycputs(x, y+ 9, col[1], "  MIDDLE STAGE");
        cons_xycputs(x, y+11, col[2], "  LARGE  STAGE");
        cons_xycputs(x, y+13, col[3], "  EXIT");
        x += 2 - TITLE_CUR_W;
        cons_xycputs(x, y+7+s_map_level*2, COL_CHOOSE, STR_TITLE_CUR);
        cons_setRefreshRect(1, x, y+7, 14, 7);      // 描画更新範囲設定.
    }
}

/// 開始.
///
static void draw_gameStart(void) {
    draw_map();
    //draw_status();
}

/// プレイ中表示.
///
static void draw_gamePlay(void) {
    // state変更か再描画リクエストがあれば、描画.
    if (s_play_draw_rq || s_draw_state != s_draw_prev_state) {
        draw_map();
        draw_cursor();
        cons_setRefreshRect(0, s_draw_map_ofs_x, s_draw_map_ofs_y, SCR_X_SCALE(s_map_w), s_map_h);
    }
    draw_status();  // 情報表示更新.(時計のため毎フレーム).
}

/// 勝利表示.
///
static void draw_gameWin(void)
{
    if (s_draw_state != s_draw_prev_state) {
        draw_map();
        draw_status();
        s_draw_tick_0 = cons_tick();
    } else {
        static char const msg[]  = "  C O N G R A T U L A T I O N S !   ";
        static char const spcs[] = "                                    ";
        char*  frame = STR_CONG_FRAME;
        uint_t  i;
        pos_t   w  = 36;
        pos_t   h  = 6;
        pos_t   x0 = (cons_screenWidth()  - w) / 2;
        pos_t   y0 = (cons_screenHeight() - h) / 2;
        uint_t  dt = cons_tick() - s_draw_tick_0;
        cons_setRefreshRect(3, x0, y0, w, h);   // 動いている範囲の描画更新.
        cons_setcolor(COL_CONG_FRAME);
        if (dt < 9) {
            cons_xyputs(x0, y0+2, frame);
        } else if (dt < 15) {
            cons_xyputs(x0, y0+1, frame);
            cons_xyputs(x0, y0+2, spcs);
            cons_xyputs(x0, y0+3, frame);
        } else if (dt < 21) {
            cons_xyputs(x0, y0+0, frame);
            for (i = 1; i <= 3; ++i)
                cons_xyputs(x0, y0+i, spcs);
            cons_xyputs(x0, y0+4, frame);
        } else {
            cons_xyputs(x0, y0+0, frame);
            cons_xyputs(x0, y0+1, spcs);
            cons_xyputs(x0, y0+3, spcs);
            cons_xyputs(x0, y0+4, frame);
            cons_xycputs( x0, y0+2, COL_CONG_1, msg);
            if (dt <= 27) {
            } else if (dt < 30+30) {
                int n = dt - 30;
                char buf[8] = { 0 };
                memcpy(buf, msg+1 + n, 4);
                cons_xycputs(x0+1 + n, y0+2, COL_CONG_2, buf);
            } else if (dt < 30+30 + 15) {
            } else {
                cons_xycputs(x0, y0+2, COL_CONG_2, msg);
                game_enableKey_fromDraw();
            }
        }
    }
}

/// ゲームオーバー表示.
///
static void draw_gameOver(void) {
    static char const gameover[] = "   G A M E  O V E R   ";
    enum { gameover_len = sizeof(gameover) - 1 };
    static char const sels[2][3][8] = {
        { " Retry ", " Title ", " Exit ", },
        { "[Retry]", "[Title]", "[Exit]", },
    };
    static uint8_t const col[2] = { COL_DEFAULT, COL_CHOOSE, };
    pos_t   x, y;

    if (s_step == 1) {
        draw_status();
        draw_map();

        y = s_draw_map_ofs_y-3;
        if (s_map_level == 0)
            --y;
        if (y < 0)
            y = 0;
        x = s_draw_map_ofs_x + ((SCR_X_SCALE(s_map_w) - gameover_len) >> 1);
        if (x < 0)
            x = 0;
        cons_xycputs(x, y, COL_GAMEOVER, gameover);
        s_draw_tick_0 = cons_tick();
    } else if (s_step == 2) {
        if (cons_tick() - s_draw_tick_0 >= CONS_MSEC_TO_TICK(100))
            game_enableKey_fromDraw();
    } else if (s_step == 3) {
        uint8_t n;
        y = s_draw_map_ofs_y+s_map_h+1;
        if (y > cons_screenHeight())
            y = cons_screenHeight()-1;
        x = s_draw_map_ofs_x + ((SCR_X_SCALE(s_map_w) - 22) >> 1);
        x += 1;
        if (x < 0)
            x = 0;
        n = (s_choise == 0);
        cons_xycputs(x      , y, col[n], sels[n][0]);
        n = (s_choise == 1);
        cons_xycputs(x +   7, y, col[n], sels[n][1]);
        n = (s_choise == 2);
        cons_xycputs(x + 2*7, y, col[n], sels[n][2]);

        cons_setRefreshRect(3, x, y, 22, 1);    // 選択肢描画更新範囲設定.
    }
}

/// ゲーム表示.
///
void draw_game(uint8_t state) {
    s_draw_prev_state = s_draw_state;
    s_draw_state      = state;
    if (state != s_draw_prev_state) {       // stateが切り替わったら全画面描画.
        cons_clear();
    } else {                                // 同一stateでは前回からの差分描画.
        cons_setRefreshRect(0, 0,0,0,0);    // 描画更新範囲設定.
    }
    switch (state) {
    case GAME_TITLE: draw_gameTitle();  break;
    case GAME_START: draw_gameStart();  break;
    case GAME_PLAY:  draw_gamePlay();   break;
    case GAME_WIN:   draw_gameWin();    break;
    case GAME_OVER:  draw_gameOver();   break;
    default: assert(0); break;
    }
}

//-----------------------------------------------------------------------------

/// main
///
int main(void) {
    return gameMain();
}
