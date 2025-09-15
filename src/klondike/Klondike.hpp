/**
 * @file Klondike.hpp
 * @brief Solitaire:Klondike Game
 * @author Masashi Kitamura (tenka@6809.net)
 * @date 2023-12 - 2025
 * @license Boost Software License - Version 1.0
 * @note
 *   pdcurses/ncurses、pc-at dos, pc98 dos.
 */
#ifndef KLONDIKE_HPP
#define KLONDIKE_HPP

#include <cassert>
#include <cstdint>
#include <cstring>
//#include <ctime>

#if defined(__DOS__) || defined(_WIN32)
#include "../misc/static_inttype_vector.hpp"
#else
#include <vector>
#endif

#ifndef KLONDIKE_USE_DIFICULTY_MODE
 #if defined(__DOS__) && !defined(__FLAT__)
  #define KLONDIKE_USE_DIFICULTY_MODE   0
 #else
  #define KLONDIKE_USE_DIFICULTY_MODE   1
 #endif
#endif

#ifndef KLONDIKE_USE_AUTO_RUN
#if !defined(NDEBUG) || KLONDIKE_USE_DIFICULTY_MODE
 #define KLONDIKE_USE_AUTO_RUN          1
#endif
#endif

namespace klondike {
    using namespace std;

 #ifdef NDEBUG
    class Card {
        uint8_t data_;
    public:
        Card() : data_(0) { }
        Card(uint8_t val, uint8_t sui, bool face_up=false)
            : data_( (val & 0xf) | ((sui & 3) << 4) | (face_up << 6) )
        {}
        Card(Card const& r) : data_(r.data_) {}
        Card& operator=(Card const& r) { data_ = r.data_; return *this; }

        //operator uint8_t() const { return data_; }

        uint8_t value() const { return data_ & 0x0f; }      // 0:none  1..13:card  (14:joker)
        uint8_t suit()  const { return (data_ >> 4) & 3; }  // 0:Spade 1:Hart 2:Club 3:Dia
        uint8_t suitValue() const { return data_ & 0x03f; }
        uint8_t faceUp() const { return (data_ >> 6) & 1; } // 0:Hide  1:Show

        void    setFaceUp() { data_ |= (1 << 6); }
        void    resetFaceUp() { data_ &= ~(1 << 6); }
        bool    isRed() const { return suit() & 1; }

        uint8_t& data() { return data_; }
        uint8_t const& data() const { return data_; }
        void    swap(Card& r) { uint8_t t = data_; data_ = r.data_; r.data_ = t; }
    };
 #else
    struct Card {
        uint8_t value_  : 4;       // 1..13, 14=joker.
        uint8_t suit_   : 2;       // 0:Spade 1:Hart 2:Club 3:Dia
        uint8_t face_up_: 1;       // 0:Hide  1:Show
    public:
        Card(uint8_t v=0, uint8_t s=0, bool f = false)
            : value_(v), suit_(s), face_up_(f)
        { }
        Card(Card const& r) noexcept { *(uint8_t*)this = *(uint8_t const*)&r; }

        //operator uint8_t() const noexcept { return *(uint8_t const*)this; }

        uint8_t value() const noexcept { return value_; }  // 0:none  1-13:card  14:joker
        uint8_t suit() const noexcept { return suit_; }    // 0:Spade 1:Hart 2:Club 3:Dia
        uint8_t suitValue() const { return (suit_ << 4) | value_; }
        uint8_t faceUp() const noexcept { return face_up_; }

        void    setFaceUp() noexcept { face_up_ = true; }
        void    resetFaceUp() noexcept { face_up_ = false; }
        bool    isRed() const noexcept { return suit_ & 1; }

        uint8_t& data() { return *(uint8_t*)this; }
        uint8_t const& data() const { return *(uint8_t const*)this; }
        void    swap(Card& r) { uint8_t t = *(uint8_t*)this; *(uint8_t*)this = *(uint8_t*)&r; *(uint8_t*)&r = t; }
    };
 #endif
    //static_assert(sizeof(Card) == 1, "");
    inline bool operator==(const Card& l, const Card& r) { return l.data() == r.data(); }
    inline bool operator!=(const Card& l, const Card& r) { return l.data() != r.data(); }
    inline bool operator< (const Card& l, const Card& r) { return l.data() <  r.data(); }
    inline bool operator>=(const Card& l, const Card& r) { return l.data() >= r.data(); }
    inline bool operator<=(const Card& l, const Card& r) { return l.data() <= r.data(); }
    inline bool operator> (const Card& l, const Card& r) { return l.data() >  r.data(); }

    inline void swap(Card&l, Card& r) { l.swap(r); }

    //  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

    typedef uint8_t  cardsize_t;
    static const cardsize_t cards_size        = 1 * 4 * 13;
    static const cardsize_t choises_size      = 7*2+4*2+1*2 + 7+1 + 6*4 + 1;
    static const cardsize_t hint_choises_size = choises_size;

    static const cardsize_t dummy_size = 1, columns_size = 7, foundations_size = 4, waste_size = 1, stock_size = 1;
    static const cardsize_t cards_array_size = dummy_size + columns_size + foundations_size + waste_size + stock_size;

 #if defined(__DOS__)
    static const size_t  history_size = 150;
 #else
    static const size_t  history_size = 256;
    //static const size_t  history_size = 16384;
 #endif

    struct Options {
        unsigned    seed;               // Rundom seed(0:time(NULL))
        uint8_t     difficulty;         // bit0:easy. bit1:difficult. bit2:canot.
        bool        draw3cards;         // false:1 draw  true: 3 draws
        bool        semi_auto;          // semi-auto mode
        bool        hint_always;        // 0:once 1:always
        bool        auto_foundation;    // auto foundation
        bool        face_up;            // all cards face up (for debug)
        bool        debug;              // debug mode
        //bool      automode;           // auto mode
        //bool      nolog;              // no log file
        size_t      loop_count;         // auto mode loop count
    public:
        Options();
    };

 #ifdef STATIC_INTTYPE_VECTOR_DEFINED
    typedef  static_inttype_vector<Card, cards_size>    Cards;
 #else
    typedef std::vector<Card> Cards;
 #endif

    enum ChoiseId /*: uint8_t */ {
        ChoiseId_none   = 0,
        ChoiseId_column_0,
        ChoiseId_column_1,
        ChoiseId_column_2,
        ChoiseId_column_3,
        ChoiseId_column_4,
        ChoiseId_column_5,
        ChoiseId_column_6,
        ChoiseId_foundation_0,
        ChoiseId_foundation_1,
        ChoiseId_foundation_2,
        ChoiseId_foundation_3,
        ChoiseId_waste,
        ChoiseId_stock,
        ChoiseId_opt,
        ChoiseId_max_size,

        ChoiseId_opt_quit           = 0,
        ChoiseId_opt_undo           = 1,
        ChoiseId_opt_hint           = 2,
        ChoiseId_opt_semiauto       = 3,
        ChoiseId_opt_autofoundation = 4,
        ChoiseId_opt_autostep1      = 5,
        ChoiseId_opt_auto           = 6,
        ChoiseId_opt_replay         = 7,
    };

    /// One move choise.
    struct Choise {
        union {
            uint16_t        data_;
            struct {
                uint8_t     src_ : 4;
                uint8_t     dst_ : 4;
                uint8_t     num_ : 4;
                uint8_t     flg_ : 4;
            };
        };
        Choise() : data_(0) {}
        Choise(uint16_t d) : data_(d) {}
        Choise(ChoiseId s, ChoiseId d, uint8_t n, uint8_t flags=0)
            : src_(uint8_t(s)), dst_(uint8_t(d)), num_(uint8_t(n)), flg_(uint8_t(flags)) {}
        Choise(Choise const& r) { data_ = r.data_; }
        uint16_t    data() const { return data_; }
        ChoiseId    src() const { return ChoiseId(src_); }
        ChoiseId    dst() const { return ChoiseId(dst_); }
        uint8_t     num() const { return num_; }
        uint8_t     flags() const { return flg_; }

        void        clear() { data_ = 0; }
    };

    static Choise const choise_undo( ChoiseId_opt, ChoiseId_opt_undo, 0 );

 #ifdef STATIC_INTTYPE_VECTOR_DEFINED
    typedef static_inttype_vector<Choise, size_t(hint_choises_size)> Choises;
    typedef static_inttype_vector<Choise, history_size>              HisChoises;
    typedef static_inttype_vector<uint32_t, history_size>            PrioChoises;
 #else
    typedef std::vector<Choise>     Choises;
    typedef std::vector<Choise>     HisChoises;
    typedef std::vector<uint32_t>   PrioChoises;
 #endif

    //  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

    /// game model.
    class Model {
        typedef cardsize_t size_type;

        class PlayingTable {
            Cards   cardsArray_[cards_array_size];
        public:
            PlayingTable() {}

            Cards&          cards(size_t n) { assert(n < cards_array_size); return cardsArray_[n]; }
            Cards const&    cards(size_t n) const { assert(n < cards_array_size); return cardsArray_[n]; }

            Cards&          dummy() { return cardsArray_[0]; }
            Cards const&    dummy() const { return cardsArray_[0]; }
            Cards&          column(size_t n) { assert(n < columns_size); return cardsArray_[dummy_size + n]; }
            Cards const&    column(size_t n) const { return const_cast<PlayingTable*>(this)->column(n); }
            Cards&          foundation(size_t n) {assert(n<foundations_size);return cardsArray_[dummy_size+columns_size+n]; }
            Cards const&    foundation(size_t n) const { return const_cast<PlayingTable*>(this)->foundation(n); }
            Cards&          waste() { return cardsArray_[dummy_size + columns_size + foundations_size]; }
            Cards const&    waste() const { return const_cast<PlayingTable*>(this)->waste(); }
            Cards&          stock() { return cardsArray_[dummy_size + columns_size + foundations_size + waste_size]; }
            Cards const&    stock() const { return const_cast<PlayingTable*>(this)->stock(); }

            void    clear();
            void    setup(unsigned int seed);

            size_t  totalCard() const;
            bool    moveCard(ChoiseId src, ChoiseId dst, cardsize_t n);
            Card    topCard(ChoiseId id, size_t n=0) const;

        private:
            static void sets_init(Cards& cards, uint8_t sets
                , uint8_t suit_type=4, uint8_t joker_num=0, bool face_up=false) noexcept;
            static void shuffle(Cards& cards, int seed = 0) noexcept;
        };

        /// One move of history.
        struct HisEnt {
            Choise  choose_;
         #if KLONDIKE_USE_DIFICULTY_MODE
            uint8_t idx_;
            Choise  choises_[choises_size];
         #endif

            HisEnt() { clear(); }
            HisEnt(HisEnt const& r);
            HisEnt(Choise choose, Choises const& choises);

            HisEnt& operator=(HisEnt const& r);
            void clear();

            void set(Choise  choose, Choises const& choises);
            void get(Choise& choose, Choises& choises) const;
        };

        /// History.
        class History {
            HisEnt      entries_[history_size];
            size_t      next_;
            size_t      max_;
            HisEnt&     back() { return entries_[(next_ - 1) % history_size]; }
        public:
            History() : next_(0), max_(0) {}
            bool    empty() const { return next_ == 0; }
            size_t  size() const { return next_; }
            void    reset() { memset(this, 0, sizeof *this); }

         #if KLONDIKE_USE_DIFICULTY_MODE
            uint8_t choiseNumber() { return back().idx_; }
            void    setChoiseNumber(uint8_t n) {  back().idx_ = n; }
            bool    getHisChoises(HisChoises& hisChoises) const;
         #endif

            void    push(Choise choose, Choises const& choises);
            bool    canUndo();
            bool    undo(Choise& choose, Choises& choises);
        };

    public:
        Model();
        ~Model();

        Cards const&    column(size_t n) const { return playing_table_.column(n); }
        Cards const&    foundation(size_t n) const { return playing_table_.foundation(n); }
        Cards const&    waste() const { return playing_table_.waste(); }
        Cards const&    stock() const { return playing_table_.stock(); }

        Choises const&  choises() const { return choises_; }
        Choises const&  hint_choises() const { return hint_choises_; }

        Options const&  options() const { return opts_; }

        size_t count() const { return count_; }
        size_t win_count() const { return win_count_; }
        size_t lose_count() const { return total_count_ - win_count_; }
        size_t total_count() const { return total_count_; }
        size_t historySize() const { return history_.size(); }
        size_t undoCount() const { return undo_count_; }

        bool   choiseIsEmpty(ChoiseId choise_id) const;

        void   incWin()   { ++win_count_; }
        void   incTotal() { ++total_count_; }

        void    setup(Options const& opts, bool has_replay=false);
        bool    run(Choise& choise);
        bool    undo();
        bool    isWin() const;
        Choise  getBestChoise();
        Choise  autoNextChoise();
        bool    getAutoFoundationChoise(Choise& dstChoise);

     #if defined(KLONDIKE_USE_AUTO_RUN)
        uint8_t autoRun(Options const& opts, bool enable_undo, unsigned& loop_count);
     #endif
     #if KLONDIKE_USE_DIFICULTY_MODE
        Choise  getHisChoise();
        bool    hasHisChoise() const { return his_choises_.size() > 0; }
     #endif

        bool    semiAutoMove() const { return opts_.semi_auto; }
        void    setSemiAutoMove(bool md) { opts_.semi_auto = md; }
        bool    autoFoundationMove() const { return opts_.auto_foundation; }

        void    setAutoFoundationMove(bool md);

        void    checkSemiAuto(ChoiseId choise_id, Choises& result) const;
        Choise  checkSemiAuto1(ChoiseId choise_id) const;

        Card    choiseToCard(ChoiseId id, size_t n=0) const;

    private:
        static bool enableMoveToColumnCard(Card src, Card dst);
        static bool enableMoveToFoundationCard(Card src, Card dst);
        Choise      checkMoveCardChoise(Choise choise);
        Choise      checkMoveCardChoise(Choise choise, Card dst_top);
        bool        moveCardChoise(Choise adjusted_choise);
        bool        undoChoise(Choise adjusted_choise);
        void        updatePlayingTable(Choise choose);
        void        makeChoises(Choises& choises, bool for_auto_play);
        uint8_t     checkClumnMovePrio(ChoiseId src_id, ChoiseId dst_id, uint8_t bn);
        void        prioChoises_push(PrioChoises& prioChoises, uint8_t prio, Choise choise);
        void        autofoundationMskClear();
        bool        autofoundationMskGet(uint8_t n) const;
        void        autofoundationMskSet(uint8_t n, bool f);
        //static void choises_push(Choises& choises, Choise choise);

    private:
        PlayingTable    playing_table_;
        Choises         choises_;
        Choises         hint_choises_;
        Choise          choose_;
        History         history_;
        Options         opts_;
        size_t          count_;
        size_t          undo_count_;
        size_t          win_count_;
        size_t          total_count_;
        size_t          stock_empty_count_;
        bool            use_waste_;

        size_t          autofoundationMsk_[(16*4 + (sizeof(size_t)*8-1)) /(sizeof(size_t)*8)];

     #if KLONDIKE_USE_DIFICULTY_MODE
        HisChoises      his_choises_;
     #endif
    };

    //  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

    /// Game view interface.
    class View {
    public:
        virtual ~View() {}
        virtual bool    setup(Model const& model) = 0;
        virtual bool    update_begin() = 0;
        virtual bool    update_end() = 0;
        virtual bool    isPause() = 0;
        virtual Choise  input() = 0;
        virtual void    choise(Choise c) = 0;
        virtual void    autoMode(bool sw) = 0;
        virtual void    semiAutoMove(bool sw) = 0;
        virtual void    autoFoundationMove(bool sw) = 0;
        virtual void    hint(bool sw) = 0;
        virtual void    invalidMove() = 0;
        virtual bool    conguratulations() = 0;
        virtual bool    lose() = 0;
        virtual void    undo() = 0;
        virtual bool    kbHit() = 0;
    };

    //  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -

    /// Game control
    class Control {
    public:
        Control(View* view);
        ~Control();

        bool setup(Options& opt);
        bool update();

        unsigned result() const { return result_; }
        Options const& options() const { return model_.options(); }

     #ifndef NDEBUG
        bool debugHiddenPlay(Options& opt, unsigned count);
     #endif

    private:
        bool updateInGame();
        bool updateOpt(Choise choise);

     #if KLONDIKE_USE_DIFICULTY_MODE
        uint8_t setupDificulty(Options& opt);
     #endif

    private:
        Model       model_;
        View*       view_;
        int         result_;
        int8_t      auto_mode_;
        bool        hint_;
        bool        pause_;
    };

} // namespace klondike

#endif  // KLONDIKE_HPP
