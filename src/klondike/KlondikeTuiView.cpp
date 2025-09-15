/**
 * @file KlondikeTuiView.cpp
 * @brief Solitaire:Klondike TUI View
 * @author Masashi Kitamura (tenka@6809.net)
 * @date 2023-12 - 2025
 * @license Boost Software License - Version 1.0
 */

#include "KlondikeTuiView.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <cstdarg>

#include "../cons/ConScr.hpp"


namespace KlondikeTui {


#if defined(__DOS__)
enum { STR_BUF_SZ = 260 };
#else
enum { STR_BUF_SZ = 1024 };
#endif

enum { co_black  = ConScr::black , co_blue    = ConScr::blue
     , co_red    = ConScr::red   , co_magenta = ConScr::magenta
     , co_green  = ConScr::green , co_cyan    = ConScr::cyan
     , co_yellow = ConScr::yellow, co_white   = ConScr::white
};
enum { co_default= co_white , co_black_suit = co_cyan ,  co_red_suit = co_red
     , co_cursor = co_yellow, co_cursor2    = co_magenta,co_sub      = co_green
     , co_hint   = co_magenta,co_win_frame  = co_cyan ,  co_info     = co_green
};

enum { top_line = 0 };
enum { columns_line_top = top_line+7 };


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/** TUI View コンストラクタ.
 */
TuiView::TuiView()
    : text_data_(textDataIni())
{
    reinit();
    ConScr::init();
 #if defined(__PC98__)
    cons_pc98_enableSJIS(0);
 #endif
}

/** TUI View デストラクタ.
 */
TuiView::~TuiView() {
    ConScr::term();
    reinit();
}

/** テキストデータ・テーブル.
 */
TuiView::TextData const& TuiView::textDataIni() {
    static TextData const s_textDataIni = {
     #if defined(__PCAT__)
        #include "KlondikeTuiStr_cp437.hh"
     #elif defined(__PC98__)
        #include "KlondikeTuiStr_pc98.hh"
     #elif defined(CONS_USE_UNICODE)
        #include "KlondikeTuiStr_utf8.hh"
     #else
        #include "KlondikeTuiStr_ascii.hh"
     #endif
    };
    return s_textDataIni;
}

/** Tui View 初期化.
 */
bool  TuiView::setup(Model const& model) /*override*/ {
    reinit();
    model_           = &model;
    semi_auto_       = model_->semiAutoMove();
    auto_foundation_ = model_->autoFoundationMove();
    //display();
    return true;
}

/** 再初期化.
 */
void TuiView::reinit() {
    model_            = nullptr;
    choise_           = Choise();
    auto_mode_        = false;
    from_selected_    = false;
    hint_             = false;
    semi_auto_        = false;
    semi_auto_choosing_=false;
    auto_foundation_  = false;
    has_user_input_   = false;
    pause_menu_flag_  = false;
    pause_menu_index_ = 0;
    win_step_         = 0;
    off_x_            = 0;
    off_y_            = 0;
    width_            = ConScr::width();
    height_           = ConScr::height();
    prev_col_         = 0;
    semi_auto_idx_    = 0;
    from_choise_id_   = ChoiseId();
    cursor_choise_id_ = ChoiseId_foundation_0;
    memset(cursor_pos_tbl_, 0, sizeof cursor_pos_tbl_);
    memset(card_name_buf_ , 0, sizeof card_name_buf_ );
    adjustSize();
}

/** update 開始.
 */
bool TuiView::update_begin() /*override*/ {
    ConScr::updateBegin();
    choise_ = Choise();
    has_user_input_ = false;
    display();
    return true;
}

/** update 終了.
 */
bool TuiView::update_end() /*override*/ {
    ConScr::updateEnd();
    return true;
}

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -

/** ゲーム・クリア演出.
 */
bool TuiView::conguratulations() /*override*/ {
    if (win_step_ < 255)
        return true;
    if (get_ch() == ConScr::KEY_NONE)
        return true;
    pause_menu_index_ = 0;
    return false;
}

/** 負け.
 */
bool TuiView::lose() /*override*/ {
    pause_menu_index_ = 0;
    return false;
}

/** 不正な移動のとき.
 */
void TuiView::invalidMove() /*override*/ {
    //put(0, height_ - 1, "Invalid move. Please try again.");
    //refresh();
}

/** オートモード on/off
 */
void TuiView::autoMode(bool sw) /*override*/ {
    auto_mode_ = sw;
}

/** Choise設定.
 */
void TuiView::choise(Choise c) /*override*/ {
    choise_ = c;
}

/** 入力.
 */
Choise TuiView::input() /*override*/ {
    return pause_menu_flag_ ? inputPauseMenu() : inputGame();
}

/** ヒントon/off
 */
void TuiView::hint(bool sw) /*override*/ {
    hint_ = sw;
}

/** セミオート on/off
 */
void TuiView::semiAutoMove(bool sw) /*override*/ {
    semi_auto_ = sw;
}

/** 組札自動移動 on/off
 */
void TuiView::autoFoundationMove(bool sw) /*override*/ {
    auto_foundation_ = sw;
}

/** アンドゥ.
 */
void TuiView::undo() /*override*/ {
}

/** ポーズ中か.
 */
bool TuiView::isPause() /*override*/ {
    return pause_menu_flag_;
}

/** キー入力の有無.
 */
bool TuiView::kbHit() /*override*/ {
    return ConScr::kbHit();
}

#if 1
/** アウトゲーム (タイトル・メニュー)
 *  @return 0=exit  1=new game
 */
int TuiView::outGame(klondike::Options& opts) {
    displayTitle(opts);
    return inputTitle(opts);
}
#endif


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
// input
//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/** 1キー入力.
 */
int TuiView::get_ch() {
    int  ch = ConScr::getCh();
    has_user_input_ = true;
    return ch;
}

/** ゲーム入力.
 */
Choise TuiView::inputGame() {

    if (semi_auto_ && from_selected_)
        return semiAutoInputGame();     // セミオート選択時.

    // プレイヤー入力.
    int c = get_ch();
    has_user_input_ = true;

    switch (c) {
    case ConScr::KEY_UP:   case 'W': case 'w':
        handleArrowKeys(1);
        break;
    case ConScr::KEY_DOWN: case 'S': case 's':
        handleArrowKeys(2);
        break;
    case ConScr::KEY_LEFT: case 'A': case 'a':
        handleArrowKeys(3);
        break;
    case ConScr::KEY_RIGHT:case 'D': case 'd':
        handleArrowKeys(4);
        break;
    case ConScr::KEY_RETURN:
    case ConScr::KEY_SPACE: case 'Z': case 'z':
        return handleCursorInput();
    case '1': case '2': case '3': case '4': case '5': case '6': case '7':
        return handleDirectInput(ChoiseId( c - '1' + ChoiseId_column_0 ));
 #ifdef KLONDIKE_USE_ABCD_KEY
    case 'A': case 'B': case 'C': case 'D':
        return handleDirectInput(ChoiseId( c - 'A' + ChoiseId_foundation_0));
    case 'a': case 'b': case 'c': case 'd':
        return handleDirectInput(ChoiseId( c - 'a' + ChoiseId_foundation_0));
 #endif
    case '8':
        if (from_selected_) {
            Card card = model().choiseToCard(from_choise_id_);
            return handleDirectInput(ChoiseId(unsigned(ChoiseId_foundation_0) + card.suit()));
        } else {
         #if 1 // 選択中の'8'は移動に留め、選択中にしない.
            cursor_choise_id_ = ChoiseId_foundation_0;
            return Choise();
         #else
            return handleDirectInput(ChoiseId_foundation_0);
         #endif
        }
    case '9':
        return handleDirectInput(ChoiseId_waste);
    case '0':
        return handleDirectInputB(ChoiseId_stock);
    case 'U': case 'u':
        from_selected_ = false;
        return Choise(ChoiseId_opt, ChoiseId_opt_undo , 0);
    case 'H': case 'h':
        return Choise(ChoiseId_opt, ChoiseId_opt_hint , 0);
    //case 'M': case 'm':
    //    return Choise(ChoiseId_opt, ChoiseId_opt_semiauto, 0);
    //    break;
    case 'T': case 't':
        if (model().options().debug) {
            from_selected_ = false;
            return Choise(ChoiseId_opt, ChoiseId_opt_autostep1, 0);
        }
        break;
    case '@':
        if (model().options().debug) {
            from_selected_ = false;
            return Choise(ChoiseId_opt, ChoiseId_opt_auto , 0);
        }
        break;
    case ConScr::KEY_ESC: case 'C': case 'c':
        if (from_selected_) {
            from_selected_ = false;
        } else {
            //return Choise(ChoiseId_opt, ChoiseId_opt_quit , 0); // ESC-Key
            pause_menu_flag_ = true;
            break; //return Choise();
        }
        break;
    default:
        break;
    }
    return Choise();
}

/** セミオートでの選択入力.
 */
Choise TuiView::semiAutoInputGame() {
    if (!semi_auto_choosing_) {
        model_->checkSemiAuto(from_choise_id_, semi_auto_choises_);
    }
    size_t choisesSize = semi_auto_choises_.size();

    if (choisesSize == 0) {
        from_selected_ = false;
        return Choise();
    } else if (choisesSize == 1) {
        from_selected_  = false;
        return semi_auto_choises_[0];
    } else {
        if (!semi_auto_choosing_) {
            semi_auto_idx_  = makePosSemiAutoChoises(from_choise_id_, semi_auto_choises_);
        }
        if (size_t(semi_auto_idx_) >= choisesSize)
            semi_auto_idx_  = 0;
        semi_auto_choosing_ = true;
        int  data           = 0;
        switch (get_ch()) {
        case ConScr::KEY_UP:    case 'W': case 'w':
        case ConScr::KEY_LEFT:  case 'A': case 'a':
            if (choisesSize)
                semi_auto_idx_ = (int)((semi_auto_idx_ - 1 + choisesSize) % choisesSize);
            break;
        case ConScr::KEY_DOWN:  case 'S': case 's':
        case ConScr::KEY_RIGHT: case 'D': case 'd':
            if (choisesSize)
                semi_auto_idx_ = (int)((semi_auto_idx_ + 1 + choisesSize) % choisesSize);
            break;
        case ConScr::KEY_RETURN:
        case ConScr::KEY_SPACE: case 'Z': case 'z':
            if (choisesSize) {
                from_selected_      = false;
                semi_auto_choosing_ = false;
                return Choise(semi_auto_choises_[semi_auto_idx_].data());
            }
            break;
        case ConScr::KEY_ESC:   case 'C': case 'c':
            from_selected_          = false;
            semi_auto_choosing_     = false;
        default:
            break;
        }
    }
    return Choise();
}

namespace {
    struct DstCols_SortCmp {
        bool is_column_;
        DstCols_SortCmp(bool is_column) : is_column_(is_column) {}
        bool operator()(Choise const& l, Choise const& r) const noexcept {
            if (!is_column_) {              // 組札の移動の時,
                ChoiseId ldst = l.dst();
                ChoiseId rdst = r.dst();
                if (ldst != rdst)           // 出力先でまずは比較.
                    return ldst < rdst;     //
            }
            ChoiseId lsrc = l.src();
            ChoiseId rsrc = r.src();
            if (lsrc != rsrc)               // ソースは全部同じハズだが念の為.
                return lsrc < rsrc;         //
            return l.num() > r.num();       // num は下からの数なので大きいほど上になる.
        }
    };
}

/** セミオート複数選択での選択肢順の並び替え.
 */
uint8_t TuiView::makePosSemiAutoChoises(ChoiseId from_choise_id, Choises& choises) {
    unsigned choises_size = unsigned(choises.size());
    if (choises_size == 0)
        return 0;

    uint16_t key = choises[0].data();

    Choises dstCols;
    dstCols.reserve(choises_size);
    Choises others;
    others.reserve(choises_size);
    Choise  dstFound;

    for (unsigned i = 0; i < choises_size; ++i) {
        ChoiseId d = choises[i].dst();
        if (d >= ChoiseId_column_0 && d <= ChoiseId_column_6) {
            dstCols.push_back(choises[i]);
        } else if (d >= ChoiseId_foundation_0 && d <= ChoiseId_foundation_3) {
            if (dstFound.data() == 0)
                dstFound = choises[i];
            else
                others.push_back(choises[i]);
        } else {
            others.push_back(choises[i]);
        }
    }
    bool is_column = from_choise_id >= ChoiseId_column_0 && from_choise_id <= ChoiseId_column_6;
    if (dstFound.data() && is_column)
        dstCols.push_back(dstFound);
    if (dstCols.size() > 1)
        std::sort(dstCols.begin(), dstCols.end(), DstCols_SortCmp(is_column));

    choises.clear();
    for (unsigned i = 0; i < dstCols.size(); ++i)
        choises.push_back(dstCols[i]);
    if (dstFound.data() && !is_column)
        choises.push_back(dstFound);

    // 念のため、列でも組札でもない選択子が残ってたら最後に追加.
    if (choises.size() < choises_size) {
        for (unsigned i = 0; i < others.size() && (choises.size() < choises_size); ++i) {
            choises.push_back(others[i]);
        }
    }

    // もともとの最有力選択肢の番号を返す.
    for (unsigned i = 0; i < choises.size(); ++i) {
        if (key == choises[i].data())
            return uint8_t(i);
    }
    return 0;
}

/** 直接(0..9)キーボード入力移動・選択.
 */
Choise TuiView::handleDirectInput(ChoiseId cursor) {
    if (!from_selected_) {      // まだ移動元が選択されていない.
        // 今回の場所を「移動元」にする.
        cursor_choise_id_ = cursor;
        from_choise_id_   = cursor;
        from_selected_    = !choiseIsEmpty(cursor);
        return Choise();
    } else {
        // 移動元が確定済み → 今回は移動先.
        from_selected_    = false;
        cursor_choise_id_ = cursor;
        return handleInputSub(from_choise_id_, cursor_choise_id_);
    }
}

/** 70'でのスタック直接キーボード入力・選択.
 */
Choise TuiView::handleDirectInputB(ChoiseId cursor) {
    if (!from_selected_) {
        cursor_choise_id_ = cursor;
        from_selected_    = false;
        return Choise(cursor_choise_id_, cursor_choise_id_, 0);
    } else {
        from_selected_    = false;
        return Choise();
    }
}

/** カーソル入力選択.
 */
Choise TuiView::handleCursorInput() {
    if (cursor_choise_id_ == ChoiseId_stock) {
        from_choise_id_  = cursor_choise_id_;
        from_selected_   = !choiseIsEmpty(cursor_choise_id_);
    }
    if (!from_selected_) {
        from_choise_id_  = cursor_choise_id_;
        from_selected_   = !choiseIsEmpty(cursor_choise_id_);
        return Choise();
    } else {
        from_selected_   = false;
        return handleInputSub(from_choise_id_, cursor_choise_id_);
    }
}

/** 移動先が foundation_0..3 なら、カードのスートに合わせる.
 */
Choise TuiView::handleInputSub(ChoiseId from, ChoiseId to) {
    if (ChoiseId_foundation_0 <= to && to <= ChoiseId_foundation_3) {
        Card c = model().choiseToCard(from);
        to = static_cast<ChoiseId>(ChoiseId_foundation_0 + c.suit());
    }
    return Choise(from, to, 0);
}

/** choise が 空か?
 */
bool TuiView::choiseIsEmpty(ChoiseId choise_id) const {
    if (choise_id == ChoiseId_stock)
        return false;
    return model_->choiseIsEmpty(choise_id);
}

/** カーソル移動.
  idx: 0=none 1=up 2=down 3=left 4=right
 */
void TuiView::handleArrowKeys(uint8_t idx) {
    enum { key_num = 1 + 7 + 4 + 1 + 1 };
    static ChoiseId const cursorTbl[key_num][5] = {
         // own                , up                    , down                , left             , right,
        { ChoiseId_none, },
        { ChoiseId_column_0    , ChoiseId_foundation_0, ChoiseId_foundation_0, ChoiseId_column_6, ChoiseId_column_1 },
        { ChoiseId_column_1    , ChoiseId_foundation_1, ChoiseId_foundation_1, ChoiseId_column_0, ChoiseId_column_2 },
        { ChoiseId_column_2    , ChoiseId_foundation_2, ChoiseId_foundation_2, ChoiseId_column_1, ChoiseId_column_3 },
        { ChoiseId_column_3    , ChoiseId_foundation_3, ChoiseId_foundation_3, ChoiseId_column_2, ChoiseId_column_4 },
        { ChoiseId_column_4    , ChoiseId_waste       , ChoiseId_waste       , ChoiseId_column_3, ChoiseId_column_5 },
        { ChoiseId_column_5    , ChoiseId_waste       , ChoiseId_waste       , ChoiseId_column_4, ChoiseId_column_6 },
        { ChoiseId_column_6    , ChoiseId_stock       , ChoiseId_stock       , ChoiseId_column_5, ChoiseId_column_0 },
        { ChoiseId_foundation_0, ChoiseId_column_0, ChoiseId_column_0, ChoiseId_stock       , ChoiseId_foundation_1 },
        { ChoiseId_foundation_1, ChoiseId_column_1, ChoiseId_column_1, ChoiseId_foundation_0, ChoiseId_foundation_2 },
        { ChoiseId_foundation_2, ChoiseId_column_2, ChoiseId_column_2, ChoiseId_foundation_1, ChoiseId_foundation_3 },
        { ChoiseId_foundation_3, ChoiseId_column_3, ChoiseId_column_3, ChoiseId_foundation_2, ChoiseId_waste        },
        { ChoiseId_waste       , ChoiseId_column_5, ChoiseId_column_5, ChoiseId_foundation_3, ChoiseId_stock        },
        { ChoiseId_stock       , ChoiseId_column_6, ChoiseId_column_6, ChoiseId_waste       , ChoiseId_foundation_0 },
    };
    cursor_choise_id_ = cursorTbl[ cursor_choise_id_ ][ idx ];
    //display();
}


/** ポーズメニュー入力.
 */
Choise TuiView::inputPauseMenu() {
    int c = get_ch();
    switch (c) {
    case 'U': case 'u':
        pause_menu_index_ = PMI_UNDO;
        return Choise(ChoiseId_opt, ChoiseId_opt_undo, 0);
    case 'H': case 'h':
        pause_menu_index_ = PMI_HINT;
        return Choise(ChoiseId_opt, ChoiseId_opt_hint, 0);
    case ConScr::KEY_ESC: case 'C': case 'c':
        pause_menu_index_ = PMI_BACK;
        pause_menu_flag_  = false;
        return Choise();
    case ConScr::KEY_UP: case 'W': case 'w':
        pause_menu_index_ = (pause_menu_index_ - 1 + PMI_NUM) % PMI_NUM;
        break;
    case ConScr::KEY_DOWN: case 'S': case 's':
        pause_menu_index_ = (pause_menu_index_ + 1 + PMI_NUM) % PMI_NUM;
        break;
    case ConScr::KEY_RETURN:
    case ConScr::KEY_SPACE: case 'Z': case 'z':
    case ConScr::KEY_LEFT : case 'A': case 'a':
    case ConScr::KEY_RIGHT: case 'D': case 'd':
        {
            //klondike::Options const& opts = model_->options();
            switch (pause_menu_index_) {
            case PMI_UNDO:
                return Choise(ChoiseId_opt, ChoiseId_opt_undo, 0);
            case PMI_HINT:
                return Choise(ChoiseId_opt, ChoiseId_opt_hint, 0);
            case PMI_SEMI_AUTO:
                return Choise(ChoiseId_opt, ChoiseId_opt_semiauto, 0);
            case PMI_AUTO_FOUNDATION:
                return Choise(ChoiseId_opt, ChoiseId_opt_autofoundation, 0);
            case PMI_REPLAY:
                return Choise(ChoiseId_opt, ChoiseId_opt_replay, 0);
            case PMI_TITLE:
                return Choise(ChoiseId_opt, ChoiseId_opt_quit, 1);
            case PMI_EXIT:
                return Choise(ChoiseId_opt, ChoiseId_opt_quit, 2);
            case PMI_BACK:
                pause_menu_flag_ = false;
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
    return Choise();
}


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
// display
//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/** Tui全表示.
 */
void TuiView::display() {
    //ConScr::updateBegin();

    ConScr::clear();
    adjustSize();
    setColor(co_default);
//    displayInfo();
    displayFoundations();
    displayColumns();
    displayWaste();
    displayStock();
    displayInfo();
    displaySourceMark();

    if (model_->isWin()) {          // 勝利演出.
        displayConguratulations();
    } else if (pause_menu_flag_) {  // ポーズメニュー.
        displayHint();
        displayPauseMenu();
    } else {                        // ゲーム本編.
        displayCursor();
        displayHint();
        if (semi_auto_ && semi_auto_choosing_) {
            displaySemiAutoDestHints();
            displaySemiAutoCursor();
        }
    }

 #if !defined(NDEBUG)
    debugDisp();
 #endif

    setColor();
    //ConScr::updateEnd();
}

/** set color
 */
inline void TuiView::setColor(int  co) {
    ConScr::setColor(co);
}

/** (x,y)に文字列表示.
 */
void TuiView::put(int x, int y, char const* s) {
    ConScr::xyPut(off_x_ + x, off_y_ + y, s);
}

/** スクリーンサイズから、ゲーム画面範囲を設定.
 */
void TuiView::adjustSize() {
    off_y_ = 0;
    off_x_ = 0;
    height_= ConScr::height();
    width_ = ConScr::width();
    if (width_ > 64) {
        off_x_ = (width_ - 64) / 2;
        width_ = 64;
    }
}

/** choise_id ごとのカーソル位置を設定.
 */
void TuiView::setCursorPosTbl(size_t choise_id, int x, int y) {
    cursor_pos_tbl_[choise_id][0] = x;
    cursor_pos_tbl_[choise_id][1] = y;
}

/** 情報表示.
 */
void TuiView::displayInfo() {
    Options const& opts = model_->options();
    setColor(co_info);
 #if KLONDIKE_USE_DIFICULTY_MODE
    static char const info_strs[2][4][16] = {
        { "<RANDOM/draw1>", "<EASY/draw1>", "<MEDIUM/draw1>", "<HARD/draw1>", },
        { "<RANDOM/draw3>", "<EASY/draw3>", "<MEDIUM/draw3>", "<HARD/draw3>", },
    };
    put( 0, 0, info_strs[opts.draw3cards][opts.difficulty]);
 #else
    static char const info_strs[2][16] = { "<Draw 1 card>" , "<Draw 3 cards>" };
    put(0, 0, info_strs[opts.draw3cards]);
 #endif
}

/** 組札(Foundations)表示.
 */
void TuiView::displayFoundations() {
    size_t dx = 8;
 #ifdef KLONDIKE_USE_ABCD_KEY
    static char const* const tbl[] = { "[A]Spad", "[B]Hart", "[C]Clob", "[D]Dia " };
 #elif 1
    setColor(co_sub);
    put(17, top_line, "[8]Foundations");
    setColor(co_default);
 #endif
    for (int i = 0; i < foundations_size; ++i) {
        Cards const& foundation = model().foundation(i);
     #ifdef KLONDIKE_USE_ABCD_KEY
        setColor(co_sub);
        put(i * dx, top_line, tbl[i]);
        setColor();
     #endif
        Card    card    = foundation.size() ? foundation.back() : Card();
        uint8_t frame   = (card.value() > 0) ? 1 : 3;
        bool    cursor  = (cursor_choise_id_ == ChoiseId_foundation_0 + i);
        displayCard(card, 1 + i * dx, top_line+1, frame, cursor);
        setCursorPosTbl(ChoiseId_foundation_0+i, 3+i*dx, top_line+5);
    }
}

/** 捨札(waste) 表示.
 */
void TuiView::displayWaste() {
    Cards const& waste = model().waste();
    unsigned waste_size = unsigned(waste.size());
    enum { x = 35 };
    setColor(co_sub);
 #if 0
    putf(x, top_line, "[9]Waste:%2u", waste_size);
 #else
    char buf[32];
    snprintf(buf, sizeof(buf)-1, "[9]Waste:%2u", waste_size);
    put(x, top_line, buf);
 #endif
    setColor(co_default);
    size_t n = (waste.size() < 3) ? waste.size() : 3;
    for (size_t i = 0; i < n; ++i) {
        uint8_t frame  = 2;
        bool    cursor = false;
        if (i == n - 1) {
            frame  = 1;
            cursor = (cursor_choise_id_ == ChoiseId_waste);
        }
        displayCard(waste[waste.size() - n + i], x + int(i * 4), top_line + 1, frame, cursor);
    }
    if (n == 0) {
        bool cursor = (cursor_choise_id_ == ChoiseId_waste);
        displayCard(Card(), x + 0*4, top_line+1, 3, cursor);
        n = 1;
    }
    setCursorPosTbl(ChoiseId_waste, x-2 + int(n)*4, top_line+5);
}

/** 山札(stock) 表示.
 */
void TuiView::displayStock() {
    Cards const& stock = model().stock();
    unsigned stock_size = unsigned(stock.size());
    enum { x = 50 };
    setColor(co_sub);
 #if 0
    putf(x, top_line, "[0]Stock:%2u", stock_size);
 #else
    char buf[32];
    snprintf(buf, sizeof(buf)-1, "[0]Stock:%2u", stock_size);
    put(x, top_line, buf);
 #endif
    setColor(co_default);

    uint8_t frame  = (stock.empty()) ? 3 : 0;
    bool cursor = (cursor_choise_id_ == ChoiseId_stock);
    displayCard(Card(), x+5, top_line+1, frame, cursor);
    setCursorPosTbl(ChoiseId_stock, x+7, top_line+5);
}

/** 列札表示.
 */
void TuiView::displayColumns() {
    size_t maxHeight = 0;
    for (int i = 0; i < columns_size; ++i) {
        Cards const& column = model().column(i);
        maxHeight = std::max(maxHeight, size_t(column.size()));
        for (uint8_t j = 0; j < 19; ++j)
            card_pos_[i][j].x = card_pos_[i][j].y = -1;
        card_pos_num_[i] = 0;
    }
    size_t  dx       = 9;
    size_t  line_max = height_; // - 1;
    for (unsigned col = 0; col < columns_size; ++col) {
        Cards const& column = model().column(col);
        size_t  x = 2 + col * dx;
        setColor(co_sub);
        char numb[] = { '[', char('1'+col), ']', 0 };
        put(int(x) + 1, columns_line_top-1, numb);

        setColor(co_default);

        size_t num = column.size();
        size_t hn  = 0;
        for (; hn < num && column[hn].faceUp() == false; ++hn)
            ;
        size_t  fn = num - hn;
        if (fn > 0)
            --fn;
        size_t  y = columns_line_top;

        size_t    hide_line   = 1;
        size_t    faceup_line = 1;
        if (y + hn * 2 + fn * 3 + 4 < line_max) {
            hide_line   = 2;
            faceup_line = 3;
        } else if (y + hn * 2 + fn * 2 + 4 < line_max) {
            hide_line   = 2;
            faceup_line = 2;
        } else if (y + hn * 1 + fn * 2 + 4 < line_max) {
            hide_line   = 1;
            faceup_line = 2;
        }

        xy_t*    postbl  = card_pos_[col];
        card_pos_num_[col] = (num <= 19) ? num : 19;
        for (size_t row = 0; row < num; ++row) {
            Card const& card = column[row];
            uint8_t frame  = card.faceUp() ? 1 : 0;
            bool    cursor = false;
            if (row == num-1) {
                hide_line = faceup_line = 4;
                cursor = (cursor_choise_id_ == ChoiseId_column_0 + col);
            }
            displayCard(card, int(x), int(y), frame, cursor, hide_line, faceup_line);
            postbl[row].x = short(x);
            postbl[row].y = short(y) + (faceup_line)/2;
            y += card.faceUp() ? faceup_line : hide_line;
        }
        setCursorPosTbl(ChoiseId_column_0+col, int(x) + 2, int(y));
    }
}

/** 選択元表示.
 */
void TuiView::displaySourceMark() {
    int x = cursor_pos_tbl_[from_choise_id_][0];
    int y = cursor_pos_tbl_[from_choise_id_][1];
    setColor(co_cursor2);
    char const* s = text_data_.cursor[1][(x && y && from_selected_)];
    if (x > 0 || y > 0)
        put(x-2, y, s);
}

/** 選択カーソル表示.
 */
void TuiView::displayCursor() {
    if (from_selected_) //(semi_auto_choosing_)
        return;
    int x = cursor_pos_tbl_[cursor_choise_id_][0];
    int y = cursor_pos_tbl_[cursor_choise_id_][1];
    setColor(co_cursor);
    char const* s = text_data_.cursor[0][(x && y)];
    put(x, y, s);
}

/** ヒント表示.
 */
void TuiView::displayHint() {
    if (!hint_ && !semi_auto_choosing_)
        return;
    Choises const& choises = model().hint_choises();
    for (size_t i = 0; i < choises.size(); ++i) {
        Choise const& choise = choises[i];
        ChoiseId src_id = choise.src();
        if (ChoiseId_column_0 <= src_id && src_id <= ChoiseId_column_6) {
            if (!semi_auto_choosing_ || from_choise_id_ == src_id)
                displayHintColmn(choise);
        } else {
            if (!semi_auto_choosing_)
                displayHintMark(src_id);
        }
    }
}

/** 列の札に対するヒント表示.
 */
void TuiView::displayHintColmn(Choise const& choise) {
    ChoiseId src_id = choise.src();
    uint8_t  nmove  = choise.num();
    if (nmove == 0)
        nmove = 1;

    int col = int(src_id - ChoiseId_column_0);
    uint8_t pos_num = card_pos_num_[col];
    if (pos_num == 0)
        return;

    // 下端(最後)からnmove 枚目.
    int idx = int(pos_num) - int(nmove);
    if (idx < 0)
        idx = 0;
    if (idx < int(pos_num)) {
        int16_t x = card_pos_[col][idx].x;
        int16_t y = card_pos_[col][idx].y;
        if (x >= 0 && y >= 0) {
            setColor(co_hint);
            char const* hint_mark = text_data_.cursor[4][1];
            x -= (hint_mark[1] == 0) ? 1 : 2;
            put(x, y, hint_mark);
            setColor(co_default);
        }
    }
}

/** ヒント・マーク表示. (for waste/stock/foundation)
 */
void TuiView::displayHintMark(ChoiseId choise_id) {
    int x = cursor_pos_tbl_[choise_id][0];
    int y = cursor_pos_tbl_[choise_id][1];
    setColor(co_hint);
    char const* s = text_data_.cursor[2][(x && y && hint_)];
    put(x+2, y, s);
}

/** セミオートでの移動先ヒント表示.
 */
void TuiView::displaySemiAutoDestHints() {
    if (!semi_auto_ || !semi_auto_choosing_ || semi_auto_choises_.empty())
        return;

    setColor(co_hint);
    char const* up = text_data_.cursor[2][1];

    for (size_t i = 0; i < semi_auto_choises_.size(); ++i) {
        Choise   c = semi_auto_choises_[i];
        ChoiseId d = c.dst();
        if ((d >= ChoiseId_column_0 && d <= ChoiseId_column_6)
         || (d >= ChoiseId_foundation_0 && d <= ChoiseId_foundation_3))
        {
            int x = cursor_pos_tbl_[d][0];
            int y = cursor_pos_tbl_[d][1];
            if (x && y) {
                put(x+2, y, up);
            }
        }
    }
    setColor(co_default);
}

/** セミオートでのカーソル表示.
 */
void TuiView::displaySemiAutoCursor() {
    if (!semi_auto_ || !semi_auto_choosing_)
        return;
    if (semi_auto_choises_.empty() || size_t(semi_auto_idx_) >= semi_auto_choises_.size())
        return;

    Choise c = semi_auto_choises_[semi_auto_idx_];

    char const* mark_l = text_data_.cursor[3][1];
    char const* mark_u = text_data_.cursor[0][1];
    setColor(co_cursor);

    if (c.src() >= ChoiseId_column_0 && c.src() <= ChoiseId_column_6) {
        int     col     = int(c.src() - ChoiseId_column_0);
        uint8_t pos_num = card_pos_num_[col];
        uint8_t n       = c.num();
        if (n == 0)
            n = 1;
        int idx = int(pos_num) - int(n);
        if (idx < 0)
            idx = 0;
        if (idx < int(pos_num)) {
            int16_t hx = card_pos_[col][idx].x;
            int16_t hy = card_pos_[col][idx].y;
            if (hx >= 0 && hy >= 0) {
                put(hx - 2, hy, mark_l);
            }
        }
    }
 #if 0
    else if (c.src() == ChoiseId_waste) {
        int wx = cursor_pos_tbl_[ChoiseId_waste][0];
        int wy = cursor_pos_tbl_[ChoiseId_waste][1];
        if (wx && wy) {
            put(wx, wy, mark_u);
        }
    }
 #endif

    if (c.dst() >= ChoiseId_foundation_0 && c.dst() <= ChoiseId_foundation_3) {
        int fx = cursor_pos_tbl_[c.dst()][0];
        int fy = cursor_pos_tbl_[c.dst()][1];
        if (fx && fy) {
            put(fx, fy, mark_u);
        }
    } else if (c.dst() >= ChoiseId_column_0 && c.dst() <= ChoiseId_column_6) {
        int cx = cursor_pos_tbl_[c.dst()][0];
        int cy = cursor_pos_tbl_[c.dst()][1];
        if (cx && cy) {
            put(cx, cy, mark_u);
        }
    }

    setColor(co_default);
}

/** カード表示.
 *    @param frame 0=hide 1=faceup 2=Foundation-0count 5=waste
 */
void TuiView::displayCard(Card card, int sx, int sy, uint8_t frame
        , bool cursor, size_t hide_line, size_t faceup_line)
{
    char buf[4][32] = {{0}};
    unsigned line = unsigned(frame == 0 ? hide_line : faceup_line);
    cardStr(buf, card, frame, cursor, line);

    if (line >= 1) put(sx, sy    , buf[0]);
    if (line >= 2) put(sx, sy + 1, buf[1]);
    if (line >= 3) put(sx, sy + 2, buf[2]);
    if (line >= 4) put(sx, sy + 3, buf[3]);

    if (ConScr::hasColor()) {
        //if (cursor) attroff(COLOR_PAIR(co_cursor));
        //else if (frame == 0) attroff(COLOR_PAIR(co_ura));

        if (card.faceUp()) {
            char name[32];
            int offx = (card.value() == 10) ? 1 : 2;
            int offy = (line > 1);
            cardToName(name, sizeof(name), card);
            int co_suit = (card.suit() & 1) ? co_red_suit : co_black_suit;
            setColor(co_suit);
            put(sx + offx, sy + offy, name);
            setColor(co_default);
        }
    }
}

/** カード名生成.
 */
char const* TuiView::cardToName(char nameBuf[], size_t capa, Card card, char const* pre) {
    uint8_t s = card.suit();
    uint8_t v = card.value();
    static char const* const valueTbl[] = {"0","A","2","3","4","5","6","7","8","9","10","J","Q","K"};
    //snprintf(nameBuf, capa, "%s%s%s", pre, text_data_.suit[s], valueTbl[v]);
    snprintf(nameBuf, capa, "%s%s%s", pre, valueTbl[v], text_data_.suit[s]);
    return nameBuf;
}

/** カード表示用文字列生成.
  frame: 0=hide 1=faceup 2=Foundation-0count 3=empty 4=waste
  cursor: true=cursor on
  line: 1..4 (number of lines to output)
 */
bool TuiView::cardStr(char buf[4][32], Card card, uint8_t ptn, bool cursor, unsigned line) {
    if (!buf || ptn > 4)
        return false;
    char const* const* t = text_data_.card[ptn][cursor];
    if (card.faceUp()) {
        char name[32];
        char const* prefix = (card.value() == 10) ? "" : (line <= 1) ? t[6] : t[7];
        cardToName(name, sizeof(name), card, prefix);
        if (line <= 1) {
            snprintf(buf[0], sizeof(buf[0]), t[4], name);
        } else {
            strcpy(buf[0], t[0]);
        }
        if (line >= 2) {
            snprintf(buf[1], sizeof(buf[1]), t[5], name);
        } else {
            strcpy(buf[1], t[1]);
        }
        if (line >= 3) strcpy(buf[2], t[2]);
        if (line >= 4) strcpy(buf[3], t[3]);
    } else {
        if (line >= 1) strcpy(buf[0], t[0]);
        if (line >= 2) strcpy(buf[1], t[1]);
        if (line >= 3) strcpy(buf[2], t[2]);
        if (line >= 4) strcpy(buf[3], t[3]);
    }
    return true;
}

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/** ポーズメニュー表示.
 */
void TuiView::displayPauseMenu() {
    setColor(co_default);
    int sx  = (width_  - 23) / 2;
    int sy  = (height_ - 10) / 2;
    int sx1 = sx + 1;
    int sx2 = sx + 19;

    put(sx1, sy + 1, "  [U]ndo              ");
    put(sx1, sy + 2, "  [H]int          "); pauseMenuPutOnOff(sx2, sy+2, hint_);
    put(sx1, sy + 3, "  Semi-Auto Move  "); pauseMenuPutOnOff(sx2, sy+3, semi_auto_);
    put(sx1, sy + 4, "  Auto Foundation "); pauseMenuPutOnOff(sx2, sy+4, auto_foundation_);
    put(sx1, sy + 5, "  Replay              ");
    put(sx1, sy + 6, "  Title               ");
    put(sx1, sy + 7, "  Exit                ");
    put(sx1, sy + 8, "  Back                ");

    pauseMenuPutFrame(sx, sy, 24, 10);

    setColor(co_cursor);
    put(sx+2, sy + 1 + pause_menu_index_, text_data_.cursor[3][1]);
    setColor();
}

/** ON/OFF 文字列表示.
 */
void TuiView::pauseMenuPutOnOff(unsigned sx, unsigned sy, bool sw) {
    setColor(co_cyan);
    static char const offOnStr[2][5] = { "OFF ", " ON " };
    put(sx, sy, offOnStr[sw]);
    setColor(co_default);
}

/** 文字列コピーして、最後の位置のポインタを返すStrCpy.
 */
char* TuiView::stpCpyE(char* d, char* e, char const* s) {
    while (*s && d < e)
        *d++ = *s++;
    *d = 0;
    return d;
}

/** ポーズメニューのフレーム表示.
 */
void TuiView::pauseMenuPutFrame(unsigned sx, unsigned sy, unsigned w, unsigned h) {
    setColor(co_default);
    char const* const* dia = text_data_.frame;
    char buf[STR_BUF_SZ+1];
    char* e = buf + sizeof(buf) - 1;
    char* p = stpCpyE(buf, e, dia[0]);
    for (unsigned i = 0; i < w-2; ++i)
        p = stpCpyE(p, e, dia[1]);
    p = stpCpyE(p, e, dia[2]);
    put(sx, sy + 0, buf);
    int x2 = sx + w - 1;
    for (unsigned y = sy+1; y < sy+h-1; ++y) {
        put(sx, y, dia[3]);
        put(x2, y, dia[5]);
    }
    p = stpCpyE(buf, e, dia[6]);
    for (unsigned i = 0; i < w-2; ++i)
        p = stpCpyE(p, e, dia[7]);
    p = stpCpyE(p, e, dia[8]);
    put(sx, sy+h-1, buf);
}

/** 勝利演出表示.
 */
void TuiView::displayConguratulations() {
    char const* frame_parts1 = text_data_.frame[1];
    char frame[STR_BUF_SZ+1];
    char* e = frame + sizeof(frame) - 1;
    char* p = frame;
    for (unsigned i = 0; i < 33; ++i)
        p = stpCpyE(p, e, frame_parts1);

    static char const* const msg   = " C O N G R A T U L A T I O N S !";
    static char const* const spcs  = "                                 ";
    int width  = 33;
    int height = 6;
    int startX = (width_  - width ) / 2;
    int startY = (height_ - height) / 2;
    setColor(co_win_frame);
    switch (win_step_) {
    case 0:
    case 1:
        ++win_step_;
        break;
    case 2:
    case 3:
        put(startX, startY+2, frame);
        ++win_step_;
        break;
    case 4:
    case 5:
        put(startX, startY+1, frame);
        put(startX, startY+2, spcs);
        put(startX, startY+3, frame);
        ++win_step_;
        break;
    case 6:
    case 7:
        put(startX, startY+0, frame);
        for (unsigned i = 1; i <= 3; ++i)
            put(startX, startY+i, spcs);
        put(startX, startY+4, frame);
        ++win_step_;
        break;
    default:
        put(startX, startY+0, frame);
        put(startX, startY+1, spcs);
        put(startX, startY+3, spcs);
        put(startX, startY+4, frame);
        setColor(co_magenta);
        put( startX, startY+2, msg);
        if (win_step_ <= 9) {
        } else if (win_step_ < 10 + 16/2) {
            int n = win_step_ - 10;
            char buf[8] = { 0 };
            memcpy(buf, msg + n * 4, 4);
            setColor(co_yellow);
            put(startX+n*4, startY+2, buf);
        } else if (win_step_ < 10 + 16/2 + 4) {
        } else {
            setColor(co_yellow);
            put(startX, startY+2, msg);
            win_step_ = 255;
            break;
        }
        ++win_step_;
        break;
    }
}

#if !defined(NDEBUG)
// debug display (top-left)
static void debug_xycputf(int x, int y, int c, char const* fmt, ...) {
    char buf[STR_BUF_SZ+1];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf)-1, fmt, ap);
    va_end(ap);
    ConScr::setColor(c);
    ConScr::xyPut(x, y, buf);
}
/** デバッグ表示.
 */
void TuiView::debugDisp() {
    setColor(co_yellow);
    debug_xycputf(0, height_-1, co_yellow, "[%ld]", (long)model_->options().seed);
}
#endif


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
// out game. (TuiView に間借り)
//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/** タイトルメニュー入力.
 * @return  -1=no action, 0=exit, 1=new game, -2=change option
 */
int TuiView::inputTitle(klondike::Options& opts) {
    TitleItem const& item = titleItem(pause_menu_index_);
    int     dx = 1;
    int     ch = get_ch();

    switch (ch) {
    case ConScr::KEY_RETURN:
    case ConScr::KEY_SPACE: case 'Z': case 'z':
        if (item.ret >= 0)
            return item.ret;
        break;
    case ConScr::KEY_LEFT : case 'A': case 'a':
        dx = -1;
        //[[fallthrough]];
    case ConScr::KEY_RIGHT: case 'D': case 'd':
        switch (pause_menu_index_) {
        case TI_STOCK_DRAW:
            opts.draw3cards      = !opts.draw3cards;
            break;
     #if KLONDIKE_USE_DIFICULTY_MODE
        case TI_DIFICULTY:
            opts.difficulty      = (opts.difficulty + dx + 4) & 3U;
            break;
     #endif
        case TI_SEMIAUTO:
            opts.semi_auto       = !opts.semi_auto;
            break;
        case TI_HINT_ALWAYS:
            opts.hint_always     = !opts.hint_always;
            break;
        //case TI_AUTO_FOUNDATION:
        //  opts.auto_foundation = !opts.auto_foundation;
        //  break;
        default:
            break;
        }
        break;

    case ConScr::KEY_UP: case 'W': case 'w':
        pause_menu_index_ = (pause_menu_index_ - 1 + TITLE_ITEM_SIZE) % TITLE_ITEM_SIZE;
        break;

    case ConScr::KEY_DOWN: case 'S': case 's':
        pause_menu_index_ = (pause_menu_index_ + 1 + TITLE_ITEM_SIZE) % TITLE_ITEM_SIZE;
        break;

    default:
        break;
    }
    return -1;
}

/** タイトル・メニュー項目文字列.
 */
TuiView::TitleItem const& TuiView::titleItem(unsigned idx) {
    static TitleItem const items[] = {
        { "NEW GAME"             ,   1, },
        { "Stock draw:"          ,  -1, },
     #if KLONDIKE_USE_DIFICULTY_MODE
        { "Dificulty:"           ,  -1, },
     #endif
        { "Semi-Auto Move:"      ,  -1, },
        { "Hint:"                ,  -1, },
      //{ "Auto Foundation Move:",  -1, },
        { "EXIT"                 ,   0, },
    };
    assert(idx < TITLE_ITEM_SIZE);
    return items[idx];
}

/** タイトル・メニュー表示.
 */
void TuiView::displayTitle(klondike::Options const& opts) {
    ConScr::updateBegin();
    ConScr::clear();

    //setColor(co_default);
    adjustSize();

    int startX = (width_  - 29) / 2;
    int startY = (height_ - 22) / 2;
    setColor(co_green);
    setColor(co_yellow);
    put(startX+0, startY  , "  S  O  L  I  T  A  I  R  E  ");
    setColor(co_green);
    put(startX+0, startY+3, "     : K L O N D I K E       ");
    enum { DY = 2, };
    startX = (width_  - 29) / 2;
    startY = (height_ - TITLE_ITEM_SIZE*DY) / 2 + 1;

    setColor(co_default);
    for (unsigned i = 0; i < TITLE_ITEM_SIZE; ++i) {
        TitleItem const& item = titleItem(i);
        put(startX + 3, startY + i * DY, item.name);
    }

     static char const* const offOnStr[2] = { "OFF", " ON", };
     static char const* const hintStr[2]  = { "  ONCE", "ALWAYS", };
    // static char const* const enableStr[2] = { "DISABLE", "ENABLE", };

    setColor(co_cyan);
    put(startX+21, startY+DY*TI_STOCK_DRAW     , opts.draw3cards ? "3 CARDS" : " 1 CARD");
 #if KLONDIKE_USE_DIFICULTY_MODE
    static char const* const dificultyStr[4] = { "RANDOM", "  EASY", "MEDIUM", "  HARD", };
    put(startX+22, startY+DY*TI_DIFICULTY      , dificultyStr[opts.difficulty % 4u]);
 #endif
    put(startX+25, startY+DY*TI_SEMIAUTO       , offOnStr[opts.semi_auto]);
    put(startX+22, startY+DY*TI_HINT_ALWAYS    , hintStr[opts.hint_always]);
    //put(startX+25, startY+DY*TI_AUTO_FOUNDATION, offOnStr[opts.auto_foundation]);
    setColor(co_cursor);
    put(startX+1, startY + pause_menu_index_ * DY, text_data_.cursor[3][1]);

    ConScr::updateEnd();
}

} // namespace KlondikeTui
