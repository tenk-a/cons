/**
 * @file Klondike.cpp
 * @brief Solitaire:Klondike Game
 * @author Masashi Kitamura (tenka@6809.net)
 * @date 2023-12 - 2025
 * @license Boost Software License - Version 1.0
 * @note
 *   pdcurses/ncurses、pc-at dos, pc98 dos.
 */

#include "Klondike.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cassert>
#include <algorithm>


namespace klondike {

using namespace std;

/** オプション初期化.
 */
Options::Options()
    : seed(0)
    , difficulty(0)     // 0:All 1:Easy 2:Medium 3:Hard
    , draw3cards(false)
    , semi_auto(true)
    , hint_always(false)
    , auto_foundation(false)
    , face_up(false)
    , debug(false)
    , loop_count(300)
    //, automode(false)
    //, nolog(false)
{
}


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/** 盤面クリア.
 */
void Model::PlayingTable::clear() {
    for (size_t i = 0; i < cards_array_size; ++i)
        cardsArray_[i].clear();
}

/** 盤面の全カード枚数.
 */
size_t Model::PlayingTable::totalCard() const {
    size_t  n = 0;
    for (size_t i = 0; i < cards_array_size; ++i)
        n += cardsArray_[i].size();
    return n;
}

/** カード移動.
 *  src から dst へ n枚移動.
 *  成功すれば true.
 */
bool Model::PlayingTable::moveCard(ChoiseId src, ChoiseId dst, cardsize_t n) {
    Cards& s = cardsArray_[src];
    Cards& d = cardsArray_[dst];
    size_t l = s.size();
    if (l >= n) {
        l -= n;
        d.insert(d.end(), s.begin() + l, s.end());
        s.resize(l);
        return true;
    }
    return false;
}

/** 指定した位置のカードを取得.
 *  n=0 で一番上のカード、n=1でその下のカード...
 *  nが範囲外なら value=0,suit=0 のダミーカードを返す.
 */
Card Model::PlayingTable::topCard(ChoiseId id, size_t n) const {
    Cards const& cards = cardsArray_[id];
    size_t       sz    = cards.size();
    if (n < sz)
        return cards[sz - 1 - n];
    return Card();
}

/** カードセット初期化.
 *  sets: セット数(1..)
 *  suit_type: スートタイプ(0:スペードのみ, 1:スペード・ハート, 2:赤黒, 3:全4スート, 4:全4スートxN)
 *  joker_num: ジョーカー枚数(0..)
 *  face_up: 全カード表向き(true)/裏向き(false)
 */
void Model::PlayingTable::sets_init(Cards& cards, uint8_t sets
    , uint8_t suit_type, uint8_t joker_num, bool face_up) noexcept
{
    cards.clear();
    cards.resize(sets * 4 * 13 + joker_num);
    assert(cards.size() > 0);
    Card* dst = &cards[0];
    for (uint8_t i = 0; i < sets; ++i) {
        for (uint8_t j = 0; j < 4; ++j) {
            int suit = (suit_type == 4) ? j : (suit_type >= 2) ? (j & 1) : 0;
            for (uint8_t value = 1; value <= 13; ++value) {
                *dst++ = Card(value, suit, face_up);
            }
        }
    }
    for (uint8_t i = 0; i < joker_num; ++i)
        *dst++ = Card(14, 3, face_up);
}

/** Xorshift32 乱数生成.
 */
static inline uint32_t xorshift32_rand(uint32_t& state) {
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (state = x);
}

/** Xorshift32 乱数生成(0..num-1).
 */
static inline uint32_t xorshift32_randN(uint32_t& state, uint32_t num) {
    return xorshift32_rand(state) % num;
}

/** カードシャッフル.
 */
void Model::PlayingTable::shuffle(Cards& cards, int seed) noexcept {
    if (seed == 0) {
        seed = std::time(nullptr);
        seed += (seed == 0);
    }
    size_t   num  = cards.size();
    assert(num > 0);
    uint32_t rand_state = seed;
    for (size_t i = num; --i > 1;) {
        size_t  t = xorshift32_randN(rand_state, num);
        std::swap(cards[i], cards[t]);
    }
}

/** 初期配置.
 *  seed: 乱数シード(0なら time(NULL))
 *  columns: 階段状に配置
 *  stock: 残りを裏向きで積む
 */
void Model::PlayingTable::setup(unsigned int seed) {
    clear();

    Cards deck;
    sets_init(deck, 1, 4, 0);
    assert(deck.size() > 0);

    shuffle(deck, seed);
    for (size_t i = 0; i < columns_size; ++i) {
        Cards& column = this->column(i);
        column.reserve(6 + 13);
        for (size_t j = 0; j <= i; ++j) {
            column.push_back(deck.back());
            deck.pop_back();
        }
        column.back().setFaceUp();
    }
    stock().swap(deck);
}


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

Model::HisEnt::HisEnt(HisEnt const& r) {
    memcpy(this, &r, sizeof *this);
}

Model::HisEnt::HisEnt(Choise choose, Choises const& choises) {
    set(choose, choises);
}

Model::HisEnt& Model::HisEnt::operator=(HisEnt const& r) {
    if (this != &r)
        memcpy(this, &r, sizeof *this);
    return *this;
}

void Model::HisEnt::clear() {
    memset((void*)this, 0, sizeof *this);
}

void Model::HisEnt::set(Choise choose, Choises const& choises) {
    choose_ = choose;

 #if KLONDIKE_USE_DIFICULTY_MODE
    idx_ = 0; //choose_.setIdx(0);
    size_type i = 0;
    for (; i < choises.size() && i < choises_size; ++i)
        this->choises_[i] = choises[i];
    for (; i < choises_size; ++i)
        this->choises_[i].clear();
 #endif
}

#if KLONDIKE_USE_DIFICULTY_MODE
namespace {
    void choises_push(Choises& choises, Choise choise) {
        unsigned j;
        for (j = 0; j < choises.size(); ++j) {
            if (choises[j].data() == choise.data())
                return; //break;
        }
        //if (j == choises.size())
            choises.push_back(choise);
    }
}
#endif

void Model::HisEnt::get(Choise& choose, Choises& choises) const {
    choose = choose_;

 #if KLONDIKE_USE_DIFICULTY_MODE
    choises.clear();
    for (size_type i = 0; i < choises_size; ++i) {
        Choise choise = this->choises_[i];
        if (choise.data() == 0)
            break;
        choises_push(choises, choise);
    }
 #endif
}


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

void Model::History::push(Choise choose, Choises const& choises) {
    ++next_;
    if (max_ < next_)
        max_ = next_;
    back() = HisEnt(choose, choises);
}

bool Model::History::canUndo() {
 #if 1
    return (max_ < history_size) ? (next_ > 0) : (next_ > max_ - history_size);
 #else
    return (max_ < history_size) ? (next_ > 1) : (next_ > max_ - history_size + 1);
 #endif
}

bool Model::History::undo(Choise& choose, Choises& choises) {
    if (canUndo()) {
        back().clear();
        --next_;
        back().get(choose, choises);
        return true;
    }
    return false;
}


#if KLONDIKE_USE_DIFICULTY_MODE
bool Model::History::getHisChoises(HisChoises& hisChoises) const {
    hisChoises.clear();
    if (max_ <= history_size && next_ > 0) {
        unsigned size = next_ - 1;
        hisChoises.resize(hisChoises.capacity());
        hisChoises.resize(size);
        for (unsigned i = 0; i < size; ++i) {
            hisChoises[i] = entries_[size - i].choose_;
        }
        return true;
    }
    return false;
}
#endif


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/** Model Constructor
 */
Model::Model()
    : count_(0)
    , undo_count_(0)
    , win_count_(0)
    , total_count_(0)
    , stock_empty_count_(0)
    , use_waste_(false)
{
    autofoundationMskClear();

 #if KLONDIKE_USE_DIFICULTY_MODE
    his_choises_.reserve(history_size);
 #endif
}

/** Model Destructor
 */
Model::~Model() {
}

/** 指定した選択肢のカードが空か.
 */
bool   Model::choiseIsEmpty(ChoiseId choise_id) const {
    return playing_table_.cards(choise_id).empty();
}

/** 盤面初期設定.
 */
void Model::setup(Options const& opts, bool has_replay) {
    opts_   = opts;
    history_.reset();
    choises_.clear();
    hint_choises_.clear();
    choose_.clear();

   #if KLONDIKE_USE_DIFICULTY_MODE
    if (!has_replay)
        his_choises_.clear();
   #endif
    playing_table_.setup( opts_.seed );

    updatePlayingTable(choose_);
    count_      = 0;
    undo_count_ = 0;
    stock_empty_count_ = 0;
    use_waste_  = false;
    autofoundationMskClear();
}

/** 選択肢の実行.
 */
bool Model::run(Choise& choise) {
    choise  = checkMoveCardChoise(choise);
    choose_ = choise;
    if (choise.num() > 0) {
        if (moveCardChoise(choise)) {
            ++count_;
            updatePlayingTable(choise);
            return true;
        }
    }
    return false;
}

/**１つ戻す.
 */
bool Model::undo() {
    Choise old_choose = choose_;
    bool rc = history_.undo(choose_, choises_);
    if (rc) {
        undoChoise(old_choose);
        makeChoises(choises_     , true );  // 選択肢作成.
        makeChoises(hint_choises_, false);  // 選択肢作成.
        ++undo_count_;
        use_waste_ = true; // 暫定.
    }
    return rc;
}

/** 勝利したか.
 */
bool Model::isWin() const {
    //return playing_table_.isWin();
    Cards const* fo = &playing_table_.foundation(0);
    for (size_t i = 0; i < 4; ++i) {
        if (fo[i].size() != 13)
            return false;
    }
    return true;
}

/** その場かぎりの最善手を選ぶ.
 */
Choise Model::getBestChoise() {
    size_type sz = choises_.size();
    if (sz)
        return choises_[0];
    //sz = hint_choises_.size();
    //if (sz) return hint_choises_[0];
    return Choise();
}

/** 自動実行での次の１手を取得.
 */
Choise Model::autoNextChoise() {
    Choises&  choises   = choises_;
    size_t    sz        = choises.size();
 #if KLONDIKE_USE_DIFICULTY_MODE
    uint8_t   idx       = history_.choiseNumber();
    if (idx < sz) {
        history_.setChoiseNumber(idx+1);
        return choises[idx];
    }
 #else
    if (sz)
        return choises[0];
 #endif
    if (history_.canUndo())
        return choise_undo;
    return Choise();
}

/** 組札へ移動する手を選択.
 */
bool Model::getAutoFoundationChoise(Choise& dstChoise) {
    Choises&  choises   = hint_choises_;
    size_type sz        = choises.size();
    for (unsigned i = 0; i < sz; ++i) {
        Choise  choise     = choises[i];
        uint8_t dstId      = choise.dst();
        if (ChoiseId_foundation_0 <= dstId && dstId <= ChoiseId_foundation_3) {
            ChoiseId srcId = choise.src();
            Card     card  = choiseToCard(srcId);
            uint8_t  val   = card.suitValue();
            if (val && !autofoundationMskGet(val)) {
                autofoundationMskSet(val, 1);
                dstChoise = choises[i];
                return true;
            }
        }
    }
    return false;
}

#if defined(KLONDIKE_USE_AUTO_RUN)
/** max_loop 回以内に勝利可能な場か?
 *  @param opts     difficulty, loop_count:最大ループ回数.
 *  return 0:不可 1:簡易な選択undo無で勝利. 2:勝利.
 */
uint8_t Model::autoRun(Options const& opts, bool enable_undo, unsigned& loop_count) {
    setup(opts);
    //uint8_t difficulty = opts_.difficulty;
    unsigned undo_count = 0;
    unsigned count      = loop_count;
    for (unsigned n = 0; n <= count; ++n) {
        if (isWin()) {
         #if KLONDIKE_USE_DIFICULTY_MODE
            history_.getHisChoises(his_choises_);
         #endif
            loop_count = n;
            return (undo_count > 0) ? 2 : 1;
        }
        Choise choise = autoNextChoise();
        if (choise.data() == choise_undo.data()) {
            if (enable_undo && undo()) {
                ++undo_count;
                continue;
            }
            break;
        } else if (choise.src() == ChoiseId_none) {
            break;
        }
        run(choise);
    }
    return 0;
}
#endif

#if KLONDIKE_USE_DIFICULTY_MODE
/** 履歴から１手取得.
 */
Choise Model::getHisChoise() {
    Choise choise;
    if (his_choises_.size()) {
        choise = his_choises_.back();
        his_choises_.pop_back();
    }
    return choise;
}
#endif

/** 自動組札移動on/off.
 */
void Model::setAutoFoundationMove(bool md) {
    opts_.auto_foundation = md;
    if (md)
        autofoundationMskClear();
}

/** セミオート移動の候補を取得(複数).
 */
void Model::checkSemiAuto(ChoiseId choise_id, Choises& result) const {
    result.clear();
    Choises const&  hints = hint_choises_;
    unsigned        n     = hints.size();
    for (unsigned i = 0; i < n; ++i) {
        ChoiseId src = hints[i].src();
        if (src == choise_id)
            result.push_back(hints[i]);
    }
}

/** セミオート移動の候補を取得(1手).
 */
Choise Model::checkSemiAuto1(ChoiseId choise_id) const {
    Choises const&  choises = hint_choises_;
    unsigned        n       = choises.size();
    for (unsigned i = 0; i < n; ++i) {
        ChoiseId src = choises[i].src();
        ChoiseId dst = choises[i].dst();
        if (src == choise_id && ChoiseId_foundation_0 <= dst && dst <= ChoiseId_foundation_3)
            return choises[i];
    }
    for (unsigned i = 0; i < n; ++i) {
        ChoiseId src = choises[i].src();
        if (src == choise_id)
            return choises[i];
    }
    return Choise();
}

/** 選択肢IDからカードを取得.
 */
Card Model::choiseToCard(ChoiseId id, size_t n) const {
    Cards const& cards = playing_table_.cards(id);
    if (n < cards.size())
        return cards[cards.size() - 1 - n];
    return Card();
}

/** 列への移動が可能か.
 */
bool Model::enableMoveToColumnCard(Card src, Card dst) {
    if (dst.value() == 0 && src.value() == 13)
        return true;
    if (dst.isRed() == src.isRed() || src.value() == 0)
        return false;
    return  (dst.value() == src.value() + 1) && dst.faceUp();
}

/** 組札への移動が可能か.
 */
bool Model::enableMoveToFoundationCard(Card src, Card dst) {
    if (src.value() == 1 && dst.value() == 0)
        return true;
    return dst.suit() == src.suit() && src.value() == dst.value() + 1;
}

/** 選択肢のカード移動をチェック.
 */
Choise Model::checkMoveCardChoise(Choise choise) {
    return checkMoveCardChoise(choise, Card());
}

Choise Model::checkMoveCardChoise(Choise choise, Card dst_top) {
    PlayingTable& pt    = playing_table_;
    bool        flag    = false;
    ChoiseId    src_id  = choise.src();
    ChoiseId    dst_id  = choise.dst();
    cardsize_t  num     = choise.num();
    Card        src_top = pt.topCard(src_id);
    if (dst_top.value() == 0)
        dst_top         = pt.topCard(dst_id);
    if (src_id <= ChoiseId_none) {
        num = 0;
    } else if (src_id <= ChoiseId_column_6) {
        Cards&      src    = pt.cards(src_id);
        cardsize_t  src_sz = src.size();
        if (dst_id <= ChoiseId_column_6) {
            if (num == 0) {
                cardsize_t tgt_no = dst_top.value();
                tgt_no = (tgt_no == 0) ? 13 : tgt_no - 1;
                for (num = src_sz; num > 0; --num) {
                    Card tgt = src[src_sz - num];
                    if (tgt.faceUp() && tgt.value() == tgt_no && (tgt.isRed() != dst_top.isRed() || tgt_no == 13)) {
                        break;
                    }
                }
            }
            if (num > src_sz)
                num = 0;
            if (num) {
                for (int j = num; j > 0; --j) {
                    Card cur = src[src_sz - j];
                    if (!cur.faceUp()) {
                        num = 0;
                        break;
                    }
                    if (cur.value() + 1 == dst_top.value() && cur.isRed() != dst_top.isRed()) {
                    } else if (cur.value() == 13 && dst_top.value() == 0) {
                    } else {
                        num = 0;
                        break;
                    }
                    dst_top = cur;
                }
                if (num && num < src_sz) {
                    if (src[src_sz - num - 1].faceUp() == false)
                        flag = true;
                }
            }
        } else if (dst_id <= ChoiseId_foundation_3) {
            num = enableMoveToFoundationCard(src_top, dst_top);
        } else {
            num = 0;
        }
        if (num && num < src_sz) {
            if (src[src_sz - num - 1].faceUp() == false)
                flag = true;
        }
    } else if (src_id <= ChoiseId_foundation_3) {
        if (ChoiseId_column_0 <= dst_id && dst_id <= ChoiseId_column_6) {
            if (src_top.value() == 13 && dst_top.value() == 0) {
                num = 1;
            } else {
                num = enableMoveToColumnCard(src_top, dst_top);
            }
        }
    } else if (src_id <= ChoiseId_waste) {
        if (dst_id == ChoiseId_none || dst_id == ChoiseId_waste) {
            //num = 0;
            for (ChoiseId idx = ChoiseId_column_0; idx <= ChoiseId_column_6; idx = ChoiseId(idx+1)) {
                if (enableMoveToColumnCard(src_top, pt.topCard(idx))) {
                    num    = 1;
                    dst_id = idx;
                    break;
                }
            }
            if (num == 0) {
                ChoiseId dst_id2 = dst_id; //ChoiseId(ChoiseId_foundation_0 + src_top.suit());
                if (enableMoveToFoundationCard(src_top, pt.topCard(dst_id2))) {
                    num    = 1;
                    dst_id = dst_id2;
                }
            }
        }
        if (dst_id <= ChoiseId_column_6) {
            num = enableMoveToColumnCard(src_top, dst_top);
        } else if (dst_id <= ChoiseId_foundation_3) {
            if (dst_id == ChoiseId_foundation_0 + src_top.suit())
                num = enableMoveToFoundationCard(src_top, pt.topCard(dst_id));
        }
    } else if (src_id <= ChoiseId_stock) {
        Cards&     stock = pt.stock();
        cardsize_t stock_num = stock.size() ? stock.size() : pt.waste().size();
        num    = opts_.draw3cards ? 3 : 1;
        if (num > stock_num)
            num = stock_num;
        dst_id = ChoiseId_waste;
    }
    if (num == 0)
        return Choise();
    return Choise( src_id, dst_id, num, flag );
}

/** 選択肢のカード移動を実行.
 */
bool Model::moveCardChoise(Choise adjusted_choise) {
    ChoiseId src_id = adjusted_choise.src();
    if (src_id == ChoiseId_none || src_id >= ChoiseId_opt)
        return false;
    PlayingTable& pt    = playing_table_;
    ChoiseId   dst_id   = adjusted_choise.dst();
    cardsize_t num      = adjusted_choise.num();

    if (src_id <= ChoiseId_column_6) {
        if (pt.moveCard(src_id, dst_id, num) == false)
            return false;
        Cards& cur = pt.cards(src_id);
        if (adjusted_choise.flags() & 1) {
            if (cur.size())
                cur.back().setFaceUp();
        }
    } else if (src_id <= ChoiseId_foundation_3) {
        if (pt.moveCard(src_id, dst_id, num) == false)
            return false;
    } else if (src_id <= ChoiseId_waste) {
        if (pt.moveCard(src_id, dst_id, num) == false)
            return false;
        use_waste_  = true;
    } else /*if (src_id <= ChoiseId_stock)*/ {
        Cards& waste = pt.waste();
        Cards& stock = pt.stock();
        if (stock.empty()) {
            stock.swap(waste);
            std::reverse(stock.begin(), stock.end());
            for (cardsize_t n = 0; n < stock.size(); ++n)
                stock[n].resetFaceUp();
            ++stock_empty_count_;
            use_waste_ = false;
        }
        if (pt.moveCard(src_id, ChoiseId_waste, num) == false)
            return false;
        cardsize_t sz     = cardsize_t(waste.size());
        for (cardsize_t i = 0; i < num; ++i) {
            waste[sz - 1 - i].setFaceUp();
        }
    }
    return true;
}

/** １手戻す.
 */
bool Model::undoChoise(Choise adjusted_choise) {
    ChoiseId src_id = adjusted_choise.src();
    if (src_id == ChoiseId_none || src_id >= ChoiseId_opt)
        return false;
    PlayingTable& pt    = playing_table_;
    ChoiseId   dst_id   = adjusted_choise.dst();
    cardsize_t num      = adjusted_choise.num();

    if (src_id <= ChoiseId_column_6) {
        Cards& cur = pt.cards(src_id);
        if (adjusted_choise.flags() & 1) {
            if (cur.size())
                cur.back().resetFaceUp();
        }
        if (pt.moveCard(dst_id, src_id, num) == false)
            return false;
    } else if (src_id <= ChoiseId_foundation_3) {
        if (pt.moveCard(dst_id, src_id, num) == false)
            return false;
    } else if (src_id <= ChoiseId_waste) {
        if (pt.moveCard(dst_id, src_id, num) == false)
            return false;
    } else /*if (src_id <= ChoiseId_stock)*/ {
        Cards& waste = pt.waste();
        Cards& stock = pt.stock();
        do {
            if (!waste.empty()) {
                Card card = waste.back();
                waste.pop_back();
                card.resetFaceUp();
                stock.push_back(card);
            }
        } while (--num);
        if (waste.empty()) {
            if (stock_empty_count_) {
                --stock_empty_count_;
                waste.swap(stock);
                std::reverse(waste.begin(), waste.end());
                for (uint8_t n = 0; n < waste.size(); ++n)
                    waste[n].setFaceUp();
                use_waste_ = true;
            }
        }
    }
    return true;
}

/** 盤面更新.
 */
void Model::updatePlayingTable(Choise choose) {
    makeChoises(hint_choises_, false);  // ヒント選択肢作成.
    makeChoises(choises_, true);        // 選択肢作成.
    history_.push(choose, choises_);    // 履歴保存.
}

/** その場で可能な選択肢を列挙.
 */
void Model::makeChoises(Choises& choises, bool for_auto_play) {
    PrioChoises         prioChoises;
    PlayingTable const& pt    = playing_table_;
    Cards const&        waste = pt.waste();

    // 未公開札の少ない順に調べる準備.
    enum { column_size = 7 };
    uint8_t tbl[column_size];
    for (uint8_t i = 0; i < column_size; ++i) {
        Cards const& src = pt.column(i);
        size_t       num = 0;
        for (size_t j = 0; j < src.size(); ++j)
            num += (src[j].faceUp() == false);
        tbl[i] = uint8_t((num << 4) | i);
    }
    sort(tbl, tbl+column_size);

    uint8_t fdn_min = 13;
    for (uint8_t suit = 0; suit < 4; ++suit) {
        ChoiseId src_id = ChoiseId(ChoiseId_foundation_0 + suit);
        Card    c = pt.topCard(src_id);
        uint8_t v = c.value();
        if (fdn_min > v)
            fdn_min = v;
    }

    // 捨て札から組札へ移動可能かチェック.
    if (waste.size()) {
        Card     src    = waste.back();
        ChoiseId dst_id = ChoiseId(ChoiseId_foundation_0 + src.suit());
        Card     dst    = pt.topCard(dst_id);
        if (src.value() == dst.value() + 1) {
            uint8_t prio = (dst.value() == fdn_min || !for_auto_play) ? 250 : 40;
            prioChoises_push(prioChoises, prio, Choise( ChoiseId_waste, dst_id, 1, 0 ) );
        }
    }

    // 列から組札へ移動可能かチェック.
    for (uint8_t n = 0; n < column_size; ++n) {
        uint8_t  i      = tbl[n] & 7;
        ChoiseId src_id = ChoiseId(ChoiseId_column_0 + i);
        Card     src    = pt.topCard(src_id);
        ChoiseId dst_id = ChoiseId(ChoiseId_foundation_0 + src.suit());
        Card     dst    = pt.topCard(dst_id);
        if (src.value() > 0) {
            Choise   rc = checkMoveCardChoise(Choise(src_id, dst_id, 0));
            if (rc.num()) {
                if (dst.value() == fdn_min) {
                    prioChoises_push(prioChoises, 240, rc);
                } else {
                    uint8_t prio = checkClumnMovePrio(src_id, dst_id, fdn_min);
                    if (prio > 1 || !for_auto_play)
                        prioChoises_push(prioChoises, 100+prio, rc);
                }
            }
        }
    }

    // 列から列へ移動可能かチェック.
    for (uint8_t n = 0; n < column_size; ++n) {
        uint8_t      i      = tbl[n] & 7;
        ChoiseId     src_id = ChoiseId(ChoiseId_column_0 + i);
        for (uint8_t j = 0; j < column_size; ++j) {
            if (i == j)
                continue;
            ChoiseId dst_id = ChoiseId(ChoiseId_column_0 + j);
            Choise   rc     = checkMoveCardChoise(Choise(src_id, dst_id, 0));
            if (rc.num() > 0 && (!for_auto_play || rc.flags())) {
                uint8_t prio= checkClumnMovePrio(src_id, dst_id, fdn_min);
                if (prio > 1 || !for_auto_play)
                    prioChoises_push(prioChoises, 100+prio, rc);
            }
        }
    }

    // 捨て札から列へ移動可能かチェック.
    if (waste.size()) {
        for (ChoiseId dst_id = ChoiseId_column_0; dst_id <= ChoiseId_column_6; dst_id = ChoiseId(dst_id+1)) {
            Choise   rc = checkMoveCardChoise(Choise(ChoiseId_waste, dst_id, 1));
            if (rc.num())
                prioChoises_push(prioChoises, 60, rc);
        }
    }

    // 組札から列への移動可能かチェック.
    if (for_auto_play == false) {   // 自動のときはあえて使わない.
        for (ChoiseId src_id = ChoiseId_foundation_0; src_id <= ChoiseId_foundation_3; src_id = ChoiseId(src_id+1)) {
            for (ChoiseId dst_id = ChoiseId_column_0; dst_id <= ChoiseId_column_6; dst_id = ChoiseId(dst_id+1)) {
                Choise   rc = checkMoveCardChoise(Choise(src_id, dst_id, 0));
                if (rc.num()) {
                    prioChoises_push(prioChoises, 20, rc);
                } else {
                    Card src = pt.topCard(src_id);
                    Card dst = pt.topCard(dst_id);
                    if (src.value() == 13 && dst.value() == 0) {
                        prioChoises_push(prioChoises, 20, Choise(src_id, dst_id, 1));
                        break;
                    }
                }
            }
        }
    }

    // 山札から引札/捨て札を出せるかチェック.
    cardsize_t stock_size = cardsize_t(pt.stock().size());
    if (stock_size || use_waste_) {
        uint8_t draw_num  = opts_.draw3cards ? 3 : 1;
        uint8_t stock_num = stock_size ? stock_size : waste.size();
        if (draw_num > stock_num)
            draw_num = stock_num;
        if (draw_num)
            prioChoises_push(prioChoises, 1, Choise(ChoiseId_stock, ChoiseId_stock, draw_num));
    }

    choises.clear();
    unsigned len = unsigned(prioChoises.size());
    if (len > 0) {
        sort(&prioChoises[0], &prioChoises[0] + len);
        for (int i = int(len); --i >= 0;) {
            choises.push_back(uint16_t(prioChoises[i]));
        }
    }
}

/** 列移動でのプライオリティ計算.
 *  移動後、札が開いたり新たなトップが移動可能だったらプライオリティを高くする.
 */
uint8_t Model::checkClumnMovePrio(ChoiseId src_id, ChoiseId dst_id, uint8_t fdn_min) {
    PlayingTable const&  pt = playing_table_;
    Cards const& src_clm    = pt.cards(src_id);
    size_t      src_clm_n   = src_clm.size();
    Choise      rc          = checkMoveCardChoise(Choise(src_id, dst_id, 0));
    uint8_t     rc_n        = rc.num();
    Card        new_src     = pt.topCard(src_id, rc_n); // 移動後の列のトップ.
    uint8_t     prio        = 0;
    if (rc_n >= src_clm_n) {                // 列の長さの移動なら、列全体の移動.
        Card    dst         = pt.topCard(dst_id);
        if (dst.value() > 0 || (ChoiseId_foundation_0 <= dst_id && dst_id <= ChoiseId_foundation_3)) {
            // Kが出揃ってない状態で空列ができるのなら高prio.
            uint8_t k_n = 0, e_n = 0;
            for (uint8_t c = ChoiseId_column_0; c <= ChoiseId_column_6; ++c) {
                if (c != src_id) {
                    Cards const& cards = pt.cards(c);
                    if (cards.size() == 0)
                        ++e_n;
                    else if (cards[0].value() == 13 && cards[0].faceUp())
                        ++k_n;
                }
            }
            prio = (k_n < 4 && e_n < (4-k_n)) ? 26 : 0;
        } else {
            prio = 1;
        }
    } else if (new_src.value() > 0) {       // 列途中からの移動.
        // 新しくトップになるカードの状態をチェック.
        if (new_src.faceUp() == false) {
            prio = 25;                      // 新しいトップ札は、裏から表になる.
        } else {
            if (new_src.value() == 13) {    // Kが途中にある状態で.
                uint8_t e_n = 0;
                for (uint8_t c = ChoiseId_column_0; c <= ChoiseId_column_6; ++c) {
                    if (c != src_id) {
                        Cards const& cards = pt.cards(c);
                        if (cards.size() == 0) {
                            prio = 24;      // 新しいトップ札Kが空列に移動可能.
                            break;
                        }
                    }
                }
            }
            if (prio == 0) {
                for (uint8_t c = ChoiseId_column_0; c <= ChoiseId_column_6; ++c) {
                    if (c != src_id && c != dst_id) {
                        Card t = pt.topCard(ChoiseId(c));
                        if (enableMoveToColumnCard(new_src, t)) {
                            prio = 22;      // 新しいトップ札が列移動可能か.
                            break;
                        }
                        Choise rc2 = checkMoveCardChoise(Choise(ChoiseId(c), src_id, 0), new_src);
                        if (rc2.num() > 0) {
                            prio = 21;      // 新しいトップ札への列移動が可能か.
                            break;
                        }
                    }
                }
            }
            if (prio == 0) {
                ChoiseId fdn_id  = ChoiseId(ChoiseId_foundation_0 + new_src.suit());
                Card     fdn_top = pt.topCard(fdn_id);
                if (enableMoveToFoundationCard(new_src, fdn_top))   // 組札移動可能.
                    prio = (fdn_top.value() == fdn_min) ? 23 : 20;  // 新しいトップ札が組札へ移動可能.
            }
        }
    }
    if (prio == 0 && rc.num())
        prio = 2 + rc.num();                // 動かせる札が多いほうをプライオリティ上げておく.
    return prio;
}

/** ダブっていない選択肢の追加.
 */
void Model::prioChoises_push(PrioChoises& prioChoises, uint8_t prio, Choise choise) {
    uint16_t choise_data = choise.data();
    unsigned n           = unsigned(prioChoises.size());
    // ダブりチェック.
    for (unsigned i = 0; i < n; ++i) {
        if (uint16_t(prioChoises[i]) == choise_data)
            return;
    }
    n = (n < 255) ? 255 - n : 0;
    uint32_t data = (uint32_t(prio) << 24) | (uint32_t(n) << 16) | choise_data;
    prioChoises.push_back(data);
}

/** 自動組札移動マスクのクリア.
 */
void Model::autofoundationMskClear() {
    memset(autofoundationMsk_, 0, sizeof autofoundationMsk_);
}

/** 自動組札移動マスクの取得/設定.
 */
bool Model::autofoundationMskGet(uint8_t n) const {
    enum { N = sizeof(size_t) * 8 };
    uint8_t j = n / N;
    uint8_t i = n % N;
    return bool((autofoundationMsk_[j] >> i) & 1u);
}

/** 自動組札移動マスクの取得/設定.
 */
void Model::autofoundationMskSet(uint8_t n, bool f) {
    enum { N = sizeof(size_t) * 8 };
    size_t& v = autofoundationMsk_[n / N];
    uint8_t i = n % N;
    size_t  m = size_t(1) << i;
    if (f) v |=  m;
    else   v &= ~m;
}


// -    -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -
// コントロール.
// -    -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

/** コントロール コンストラクタ.
 */
Control::Control(View* view)
    : view_(view)
    , result_(0)
    , auto_mode_(0)
    , hint_(false)
    , pause_(false)
{
    assert(view != nullptr);
}

/** コントロール デストラクタ.
 */
Control::~Control() {
}

/** コントロール 準備.
 */
bool Control::setup(Options& opt) {
    if (opt.seed == 0)
        opt.seed = std::time(nullptr);
 #if KLONDIKE_USE_DIFICULTY_MODE
    uint8_t rt = setupDificulty(opt);
    model_.setup(opt, rt != 0);
 #else
    model_.setup(opt);
 #endif
    view_->setup(model_);
    auto_mode_      = 0;
    hint_           = false;
    result_         = 0;
    return true;
}

/** ターン(or毎フレーム)の更新.
 */
bool Control::update() {
    bool rc = true;
    if (view_->update_begin() == false)
        return false;
    pause_ = view_->isPause();
    rc &= updateInGame();
    rc &= view_->update_end();
    return rc;
}

/** ゲーム中の更新.
 */
bool Control::updateInGame() {
    if (model_.isWin()) {
        if (view_->conguratulations())
            return true;
        model_.incWin();
        model_.incTotal();
        result_ = 4;
        return false;
    }

    Choise choise;
    if (!pause_ && auto_mode_) {   // 自動モード.
        if (auto_mode_ > 0) {   // 1 回自動.
            choise = model_.getBestChoise();
            --auto_mode_;
        } else if (auto_mode_ < 0) {    // 全自動.
         #if KLONDIKE_USE_DIFICULTY_MODE
            if (auto_mode_ == -2) {
                choise = model_.getHisChoise();
            } else
         #endif
            {
                choise = model_.autoNextChoise();
                if (choise.data() == choise_undo.data()) {
                 #if 0
                    if (model_.undo()) {
                        view_->undo();
                        return true;
                    } else
                 #endif
                    {
                        choise = Choise();
                        auto_mode_ = 0;
                    }
                }
            }
            if (view_->kbHit())
                auto_mode_ = 0;
        }
    } else if (!pause_ && model_.autoFoundationMove() && model_.getAutoFoundationChoise(choise)) {
        ;
    } else {    // ユーザー入力.
        choise = view_->input();
    }

    if (int(choise.src()) && int(choise.src()) < int(ChoiseId_opt)) {
        if (model_.run(choise)) {
            view_->choise(choise);
        } else {
            auto_mode_ = 0;
            view_->autoMode(false);
        }
        if (model_.options().hint_always == false) {
            hint_ = false;
            view_->hint(hint_);
        }
    } else if (choise.src() == ChoiseId_opt) {
        auto_mode_ = 0;
        if (updateOpt(choise) == false)
            return false;
        view_->autoMode(auto_mode_ < 0);
    } else {
        auto_mode_ = 0;
        view_->autoMode(false);
    }
    return true;
}

/** オプション選択の更新.
 */
bool Control::updateOpt(Choise choise) {
    switch (choise.dst()) {
    case ChoiseId_opt_quit:         // 終了.
        //view_->quit();
        view_->lose();
        model_.incTotal();
        result_ = choise.num_;
        return false;
    case ChoiseId_opt_replay:       // 再プレイ.
        //view_->quit();
        view_->lose();
        model_.incTotal();
        result_ = 0;
        return false;
    case ChoiseId_opt_undo:         // １つ戻す.
        if (model_.undo())
            view_->undo();
        break;
    case ChoiseId_opt_hint:         // ヒント表示.
        hint_ = !hint_;
        view_->hint(hint_);
        break;
    case ChoiseId_opt_semiauto:     // セミオート移動.
        model_.setSemiAutoMove(!model_.semiAutoMove());
        view_->semiAutoMove(model_.semiAutoMove());
        break;
    case ChoiseId_opt_autofoundation:   // 自動組札移動.
        model_.setAutoFoundationMove(!model_.autoFoundationMove());
        view_->autoFoundationMove(model_.autoFoundationMove());
        break;
    case ChoiseId_opt_autostep1:    // 1手自動移動.
        auto_mode_ = 1;
        break;
    case ChoiseId_opt_auto:         // 全自動移動.
        auto_mode_ = -1;
     #if KLONDIKE_USE_DIFICULTY_MODE
        if (model_.count() == 0 && model_.hasHisChoise()) {
            auto_mode_ = -2;
        }
     #endif
        view_->autoMode(true);
        break;
    default:
        break;
    }
    return true;
}

#if KLONDIKE_USE_DIFICULTY_MODE
/** 難易度設定モードでの準備.
 */
uint8_t Control::setupDificulty(Options& opt) {
    uint8_t difficulty = opt.difficulty;
    if (difficulty == 0 || difficulty > 3)
        return 0;
    bool    enable_undo = (difficulty > 1);
    uint8_t rc = 0;
    for (;;) {
        unsigned loop_count = (difficulty == 1) ? 181 : (difficulty == 2) ? 251 : 251;

        rc = model_.autoRun(opt, enable_undo, loop_count);

        if (difficulty == 1) {
            if (rc == 1 && loop_count <= 160)
                break;
        } else if (difficulty == 2) {
            if (rc == 2 && 100 <= loop_count && loop_count <= 250)
                break;
            if (rc == 1 && 160 <  loop_count && loop_count <= 250)
                break;
        } else if (difficulty == 3) {
            if (rc == 0 || (rc == 2 && loop_count >= 100))
                break;
        }

        ++opt.seed;
    }
    return rc;
}
#endif  // KLONDIKE_USE_DIFICULTY_MODE

#ifndef NDEBUG
/** デバッグ用: 隠しプレイ.
 */
bool Control::debugHiddenPlay(Options& opt, unsigned play_count) {
 #if defined(KLONDIKE_USE_AUTO_RUN)
    uint8_t  difficulty = opt.difficulty;
    difficulty = (difficulty < 1) ? 1 : (difficulty > 3) ? 3 : difficulty;
    printf("Debug Hidden play : seed=%lu  count=%u\n", (unsigned long)opt.seed, play_count);
    bool     enable_undo = (difficulty > 1);
    unsigned win1 = 0, win2 = 0;
    for (unsigned l = 0; l < play_count; ++l) {
        //unsigned loop_count = opt.loop_count;
        //unsigned loop_count = (difficulty == 1) ? 181 : (difficulty == 2) ? 251 : 251;
        unsigned loop_count = (difficulty == 1) ? 151 : (difficulty == 2) ? 251 : 251;
        uint8_t rc = model_.autoRun(opt, enable_undo, loop_count);
        win1 += (rc == 1);
        win2 += (rc == 2);
        ++opt.seed;
    }
    printf("win:%u(%u+%u) lose:%u  total:%u\n"
            , win1+win2, win1, win2, play_count - (win1+win2), play_count);
 #endif
    return true;
}
#endif


}   // klondike
