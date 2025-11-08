/**
 * @file KlondikeConsoleView.hpp
 * @brief Solitaire:Klondike Console View
 * @author Masashi Kitamura (tenka@6809.net)
 * @date 2023-12 - 2025
 * @license Boost Software License - Version 1.0
 */

#ifndef CONSOLE_KLONDIKE_VIEW_HPP
#define CONSOLE_KLONDIKE_VIEW_HPP

//#define USE_UNICODE_CARD

#include "Klondike.hpp"

#include <cstdio>
#include <cstdlib>
#include <cctype>

///
namespace KlondikeConsole {
    using namespace klondike;

    struct ConsoleView_TextData {
        char const* suit[4];
    public:
        ConsoleView_TextData() {
         #if defined(CONS_USE_UNICODE)
            suit[0] = "♠";
            suit[1] = "♥";
            suit[2] = "♣";
            suit[3] = "♦";
         #else
            suit[0] = "s";
            suit[1] = "h";
            suit[2] = "c";
            suit[3] = "d";
         #endif
        }
    };

    class ConsoleView : public klondike::View {
    private:
        Model const*    model_;
        Choise          choise_;
        bool            semi_auto_;
        bool            auto_foundation_;
        bool            auto_mode_;
        bool            first_done_;
        char            cardNameBuf_[64];
        char            cardNameBuf2_[64];
        ConsoleView_TextData    text_data_;
    public:
        typedef ConsoleView_TextData TextData;

        ConsoleView() {
            release();
        }
        ~ConsoleView() {}

        bool setup(Model const& model) override {
            release();
            model_ = &model;
            return true;
        }

        void release() {
            model_ = nullptr;
            choise_ = Choise();
            auto_mode_ = false;
            semi_auto_ = 0;
            first_done_ = false;
            cardNameBuf_[0] = 0;
            cardNameBuf2_[0] = 0;
        }

        bool update_begin() override {
            //printf("\f");
            if (!first_done_) {
                first_done_ = true;
                outputFirst();
            }
            if (size_t(choise_.src())) {
                printf("\n[%u] ", (unsigned)model().count());
                outputChoise(choise_);
                printf("\n");
            }
            choise_ = Choise();
            displayFoundations();
            displayColumns();
            displayWaste();
            displayStock();
            printf("\n");
            return true;
        }

        bool update_end() override {
            return true;
        }

        bool conguratulations() override {
            printf("\nCongratulations!\n\n");
            outputWinLoseTotal();
            first_done_ = false;
            return false;
        }

        bool lose() override {
            printf("\nLose\n\n");
            outputWinLoseTotal();
            first_done_ = false;
            return false;
        }

        void invalidMove() override {
            printf("Invalid move. Please try again.\n");
        }

        void autoMode(bool sw) override {
            auto_mode_ = sw;
        }

        void semiAutoMove(bool mode) override {
            semi_auto_ = mode;
            printf(" Semi-Auto Move: %s\n", mode ? "ON" : "OFF");
        }

        void autoFoundationMove(bool mode) override {
            auto_foundation_ = mode;
            printf(" Auto Foundation Move: %s\n", mode ? "ON" : "OFF");
        }

        void choise(Choise c) override {
            choise_ = c;
        }

        Choise input() override {
            printf("format: [SRC] [DST]\n"
                   "  1..7:Columns 8:Fundation 9:waste  0:Draw from stock\n"
                   "  U:undo H:hint M:semi T:step A:Auto R:Replay Q:Quit\n");
            printf("(%ld) Choose action: ", (unsigned long)model().historySize());

            char buf[130] = {0};
            unsigned char src_ch = 0, dst_ch = 0;
            for (;;) {
                if (fgets(buf, sizeof(buf), stdin) == nullptr)
                    continue;
                unsigned char* p = (unsigned char*)buf;
                while ((*p && *p <= ' ') || *p == 0x7f)
                    ++p;
                if (*p == 0)
                    continue;
                src_ch = *p++;
                while (*p > ' ')
                    ++p;
                while ((*p && *p <= ' ') || *p == 0x7f)
                    ++p;
                if (*p == 0)
                    break;
                dst_ch = *p;
                break;
            }

            src_ch = toupper(src_ch);
            dst_ch = toupper(dst_ch);
            ChoiseId src = charToChoiseId(src_ch);
            if (src == ChoiseId_none) {
                switch (src_ch) {
                case 'Q': return Choise( ChoiseId_opt, ChoiseId_opt_quit  , 2 );
                case 'R': return Choise( ChoiseId_opt, ChoiseId_opt_replay, 0 );
                case 'U': return Choise( ChoiseId_opt, ChoiseId_opt_undo  , 0 );
                case 'H': return Choise( ChoiseId_opt, ChoiseId_opt_hint  , 0 );
                case 'T': return Choise( ChoiseId_opt, ChoiseId_opt_autostep1, 0 );
                case 'A': return Choise( ChoiseId_opt, ChoiseId_opt_auto  , 0 );
                case 'M':
                    semi_auto_ = (semi_auto_ + 1) & 3;
                    return Choise( ChoiseId_opt, ChoiseId_opt_semiauto, semi_auto_);
                default:
                    ;
                }
            }
            ChoiseId dst = charToChoiseId(dst_ch);

            if (dst == ChoiseId_foundation_0 && src > ChoiseId_none) {
                if (src == ChoiseId_foundation_0)
                    return Choise();
                Card card = model().choiseToCard(src);
                dst = ChoiseId(unsigned(ChoiseId_foundation_0) + card.suit());
            } else if (src == ChoiseId_foundation_0 && ChoiseId_column_0 <= dst && dst <= ChoiseId_column_6) {
                Card card = model().choiseToCard(dst);
                src = ChoiseId(unsigned(ChoiseId_foundation_0) + card.suit());
            }

            if (dst == ChoiseId_none && (semi_auto_ & 1)) {
                Choise choise = model_->checkSemiAuto1(src);
                if (choise.src() == src) {
                    dst = choise.dst();
                }
            }

            return Choise( src, dst, 0 );
        }

        void hint(bool) override {
            Choises const& choises = model().choises();
            for (size_t i = 0; i < choises.size(); ++i) {
                Choise const& choise = choises[i];
                printf("%3u. ", (unsigned)(i + 1));
                outputChoise(choise);
            }
            if (choises.size() == 0) {
                printf("(Nothing)\n");
            }
            printf("\n");
        }

        void undo() override {
            printf("\nUndo %u\n\n",(unsigned)model().undoCount());
        }

        bool isPause() override {
            return false;
        }

        bool kbHit() override {
            return false;
        }

    private:
        Model const& model() const { assert(model_ != nullptr); return *model_; }

        void outputFirst() {
            Options const& opts = model().options();
            printf("[klondike] Seed:%d\n", opts.seed);
            printf("\tDraw-step : %d\n", opts.draw3cards?3:1);
            printf("\tDifficulty: %d\n", opts.difficulty);
            printf("\n");
        }

        void outputWinLoseTotal() {
            printf("Win: %u  Lose: %u  Total: %u\n"
                , unsigned(model().win_count()), unsigned(model().lose_count()), unsigned(model().total_count()));
        }

        bool outputChoise(Choise choose) {
            int src = int(choose.src());
            int dst = int(choose.dst());
            if (int(ChoiseId_column_0) <= src && src <= int(ChoiseId_waste)) {
                printf("Move from %s to %s.\n", choiseIdToName(src), choiseIdToName(dst));
            } else if (src == int(ChoiseId_stock)) {
                printf("Draw from stock.\n");
            }
            return true;
        }

        static char const* choiseIdToName(size_t id) {
            static char const* const tbl[] = {
                "none",
                "Column[1]",
                "Column[2]",
                "Column[3]",
                "Column[4]",
                "Column[5]",
                "Column[6]",
                "Column[7]",
                "Foundation", //"[SPADE]",
                "Foundation", //[HART]",
                "Foundation", //[CLOVER]",
                "Foundation", //[DIAMOND]",
                "Waste",
                "Draw stock",
                "(ex)",
                "",
            };
            return tbl[id & 15];
        }

        ChoiseId charToChoiseId(unsigned char c) {
            if (c == 0 || c >= 0x80)
                return ChoiseId_none;
            c = toupper(c);
            switch (c) {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                return ChoiseId(uint8_t(ChoiseId_column_0) + c - '1');
            case '8':
                return ChoiseId_foundation_0;
            case '9':
                return ChoiseId_waste;
            case '0':
                return ChoiseId_stock;
            default:
                return ChoiseId_none;
            }
        }

        void displayColumns() {
            for (int i = columns_size; --i >= 0;) {
                printf("Column %u: ", i+1);
                Cards const& cards = model().column(i);
                for (size_t j = 0; j < cards.size(); ++j) {
                    Card const& card = cards[j];
                    printf("%s ", cardToName(card));
                }
                printf("\n");
            }
        }

        void displayFoundations() {
            printf("Foundations: ");
            for (size_t i = 0; i < foundations_size; ++i) {
                Cards const& fundation = model().foundation(i);
                if (!fundation.empty()) {
                    printf("%s ", cardToName(fundation.back()));
                } else {
                    printf("%s ", card_pattern());
                }
            }
            printf("\n");
        }

        void displayWaste() {
            Cards const& waste = model().waste();
            printf("Waste: ");
            for (size_t i = 0; i < waste.size(); ++i) {
                printf("%s ", cardToName(waste[i]));
            }
            printf("\n");
        }

        void displayStock() {
            Cards const& stock = model().stock();
            printf("Stock: ");
            for (int i = int(stock.size()); --i >= 0;) {
                printf("%s ", cardToName(stock[i]));
            }
            printf("\n");
        }

        char const* cardToName(Card card) {
            if (this->model().options().debug) {
                char const* name = cardToName(card, true);
                if (card.faceUp()) {
                    snprintf(cardNameBuf2_, sizeof cardNameBuf2_, "'%s'", name);
                } else {
                    snprintf(cardNameBuf2_, sizeof cardNameBuf2_, "[%s]", name);
                }
                return cardNameBuf2_;
            } else {
                char const* name = cardToName(card, card.faceUp());
                return name;
            }
        }

        char const* cardToName(Card card, uint8_t face_up) {
            uint8_t v = card.value();
            face_up  |= card.faceUp();
         #if defined(USE_UNICODE_CARD)
            uint8_t s = card.suit();
            static char const* const card_pt[] = {
                "🂡🂢🂣🂤🂥🂦🂧🂨🂩🂪🂫🂬🂭🂮",
                "🂱🂲🂳🂴🂵🂶🂷🂸🂹🂺🂻🂼🂽🂾",
                "🃁🃂🃃🃄🃅🃆🃇🃈🃉🃊🃋🃌🃍🃎",
                "🃑🃒🃓🃔🃕🃖🃗🃘🃙🃚🃛🃜🃝🃞",
            };
            char const* pt = card_pt[s];
            snprintf(cardNameBuf_, sizeof cardNameBuf_, "%*.*s", 4, 4, pt + (v-1) * 4);
         #else
            if (v > 0 && face_up) {
                static const char valueTbl[] = "0A234567891JQK";
                if (v <= 13) {
                    snprintf(cardNameBuf_, sizeof cardNameBuf_, "%s%c%s"
                            , text_data_.suit[card.suit()], valueTbl[v], (v==10) ? "0":" ");
                } else {
                    snprintf(cardNameBuf_, sizeof cardNameBuf_, "??");
                }
            } else {
                snprintf(cardNameBuf_, sizeof cardNameBuf_, "%s", card_pattern());
            }
         #endif
            return cardNameBuf_;
        }

        static char const* card_pattern() {
         #if defined(USE_UNICODE_CARD)
            return "🂠";
         #else
            return "###";
         #endif
        }
    };

}   // klondike

#endif  // CONSOLE_KLONDIKE_VIEW_HPP
