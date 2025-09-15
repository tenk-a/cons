/**
 * @file Klondike.hpp
 * @brief Solitaire:Klondike TUI View
 * @author Masashi Kitamura (tenka@6809.net)
 * @date 2023-12 - 2025
 * @license Boost Software License - Version 1.0
 */
#ifndef TUI_KLONDIKE_VIEW_HPP
#define TUI_KLONDIKE_VIEW_HPP

#include "Klondike.hpp"

namespace KlondikeTui {
    using namespace klondike;

    class TuiView : public klondike::View {
    public:
        struct TextData {
            char const* suit[4];
            char const* frame[9];
            char const* cursor[5][2];
            char const* card[4][2][8];
        };

        TuiView();
        ~TuiView();

        bool    setup(Model const& model) override;

        bool    update_begin() override;
        bool    update_end() override;
        bool    conguratulations() override;

        bool    lose() override;
        void    invalidMove() override;
        void    autoMode(bool sw) override;
        void    choise(Choise c) override;
        Choise  input() override;
        void    hint(bool sw) override;
        void    semiAutoMove(bool sw) override;
        void    autoFoundationMove(bool sw) override;
        void    undo() override;
        bool    isPause() override;
        bool    kbHit() override;

     #if 1
        int     outGame(klondike::Options& opts);
     #endif

    private:
        enum {
            PMI_UNDO            = 0,
            PMI_HINT            = 1,
            PMI_SEMI_AUTO       = 2,
            PMI_AUTO_FOUNDATION = 3,
            PMI_REPLAY          = 4,
            PMI_TITLE           = 5,
            PMI_EXIT            = 6,
            PMI_BACK            = 7,
            PMI_NUM             = 8,
        };
        struct xy_t { short x, y; };

        Model const& model() const { assert(model_ != nullptr); return *model_; }

        void   reinit();

        int    get_ch();
        Choise inputGame();
        Choise semiAutoInputGame();
        Choise inputPauseMenu();
        Choise handleDirectInput(ChoiseId cursor);
        Choise handleDirectInputB(ChoiseId cursor);
        Choise handleCursorInput();
        Choise handleInputSub(ChoiseId from, ChoiseId to);
        void   handleArrowKeys(uint8_t idx);
        bool   choiseIsEmpty(ChoiseId choise_id) const;
        uint8_t makePosSemiAutoChoises(ChoiseId choise_id, Choises& choises);

        void display();
        //void putf(int x, int y, char const* fmt, ...);
        void put(int x, int y, char const* s);
        void setColor(int  co = 0);
        void adjustSize();
        void displayInfo();
        void displayFoundations();
        void displayWaste();
        void displayStock();
        void displayColumns();
        void displaySourceMark();
        void displayCursor();
        void displaySemiAutoCursor();
        void displaySemiAutoDestHints();
        void displayHint();
        void displayHintColmn(Choise const& choise);
        void displayHintMark(ChoiseId choise_id);
        void displayCard(Card card, int sx, int sy, uint8_t frame
                , bool cursor, size_t hide_line=4, size_t faceup_line=4);
        void setCursorPosTbl(size_t choise_id, int x, int y);
        char const* cardToName(char nameBuf[], size_t capa, Card card, char const* pre = "");
        bool cardStr(char buf[4][32], Card card, uint8_t ptn, bool cursor, unsigned line);

        void displayConguratulations();

        void displayPauseMenu();
        void pauseMenuPutOnOff(unsigned sx, unsigned sy, bool sw);
        void pauseMenuPutFrame(unsigned sx, unsigned sy, unsigned w, unsigned h);

        static char* stpCpyE(char* d, char* e, char const* s);

    private:    // zantei out game.
        enum {
            TI_NEW_GAME,
            TI_STOCK_DRAW,
         #if KLONDIKE_USE_DIFICULTY_MODE
            TI_DIFICULTY,
         #endif
            TI_SEMIAUTO,
            TI_HINT_ALWAYS,
          //TI_AUTO_FOUNDATION,
            TI_EXIT,

            TITLE_ITEM_SIZE,
        };

        struct TitleItem {
            char const* name;
            int8_t      ret;
        };
        static TitleItem const& titleItem(unsigned idx);
        int    inputTitle(klondike::Options& opts);
        void   displayTitle(klondike::Options const& opts);

        static TextData const& textDataIni();

     #if !defined(NDEBUG)
        void debugDisp();
     #endif

    private:
        Model const* model_;
        Choise      choise_;
        bool        hint_;
        bool        from_selected_;
        bool        auto_mode_;
        bool        semi_auto_;
        bool        semi_auto_choosing_;
        bool        auto_foundation_;
        bool        has_user_input_;
        bool        pause_menu_flag_;
        uint8_t     pause_menu_index_;
        uint8_t     win_step_;
        int         off_x_;
        int         off_y_;
        unsigned    width_;
        unsigned    height_;
        int         prev_col_;
        int         semi_auto_idx_;
        ChoiseId    from_choise_id_;
        ChoiseId    cursor_choise_id_;
        Choises     semi_auto_choises_;

        uint8_t     cursor_pos_tbl_[14][2];
        char        card_name_buf_[8];
        xy_t        card_pos_[columns_size][13+6];
        uint8_t     card_pos_num_[columns_size];
        TextData const& text_data_;
    };

} // namespace KlondikeTui

#endif // TUI_KLONDIKE_VIEW_HPP
