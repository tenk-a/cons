/**
 * @file klondike_main.cpp
 * @brief Solitaire:Klondike Game
 * @author Masashi Kitamura (tenka@6809.net)
 * @date 2023-12 - 2025
 * @license Boost Software License - Version 1.0
 */

#if defined(_WIN32)
/*
#include <windows.h>
#undef min
#undef max
*/
#else
#include <locale.h>
#include <sys/stat.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "Klondike.hpp"

#if !defined(USE_CUI)
#include "KlondikeTuiView.hpp"
#else
#include "KlondikeConsoleView.hpp"
#endif

#ifndef _MAX_PATH
#define _MAX_PATH  1024
#endif

#ifndef LOAD_BUF_BYTES
#if defined(__DOS__)
#define LOAD_BUF_BYTES  2048
#else
#define LOAD_BUF_BYTES  0x2000
#endif
#endif

using namespace std;


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -
#if 0
#if defined(_WIN32)
class ConsoleCP {
    int  save_cp_;
public:
    ConsoleCP() : save_cp_(GetConsoleOutputCP()) { SetConsoleOutputCP(65001); }
    ~ConsoleCP() { SetConsoleOutputCP(save_cp_); }
};
#elif !defined(__DOS__)
struct ConsoleCP {
    ConsoleCP() { setlocale(LC_ALL, ""); }
};
#else
struct ConsoleCP { };
#endif
#endif

//  -   -   -   -   -   -   -   -   -   -   -   -   -   -
/// Klondike Game
class KlondikeGame {
public:
    klondike::Options   opt;

    KlondikeGame()
        : view_(NULL), ctrl_(NULL)
    {
    }

    ~KlondikeGame() {
        delete view_;
        delete ctrl_;
    }

    void setup() {
        if (opt.debug)
            printf("sizeof(klondike::Model)=%u\n", (unsigned)sizeof(klondike::Model));

     #if !defined(USE_CUI)
        view_   = new KlondikeTui::TuiView();
     #else
        view_   = new KlondikeConsole::ConsoleView();
     #endif
        ctrl_   = new klondike::Control(view_);
        if (ctrl_)
            ctrl_->setup(opt);
    }

    void reset() {
        if (ctrl_)
            ctrl_->setup(opt);
    }

    bool update() {
        if (!ctrl_)
            return false;
        return ctrl_->update();
    }

    int  outGame() {
     #if !defined(USE_CUI)
        return ((KlondikeTui::TuiView*)view_)->outGame(this->opt);
     #else
        return 1;
     #endif
    }

    unsigned result() const {
        return ctrl_->result();
    }

    void updateOptions() {
        //this->opt = ctrl_->options();
        this->opt.semi_auto = ctrl_->options().semi_auto;
    }

    bool debugHiddenPlay(unsigned count) {
     #ifndef NDEBUG
        return ctrl_->debugHiddenPlay(this->opt, count);
     #else
        return true;
     #endif
    }

private:
    klondike::View*     view_;
    klondike::Control*  ctrl_;
};


//  -   -   -   -   -   -   -   -   -   -   -   -   -   -
/// Game

class App {
    char const*     appname_;
    KlondikeGame    klondike_;
    unsigned        debug_hidden_play_count_;
public:
    App()
        : appname_("")
        , debug_hidden_play_count_(0)
    { }
    ~App() { }

    int main(int argc, char* argv[]) {
        appname_ = argv[0];

        loadDat();
        loadIni();

        for (int i = 1; i < argc; ++i) {
            char* arg = argv[i];
            if (checkOpts(arg) == false)
                return 1;
        }

        klondike_.setup();

     #ifndef NDEBUG
        if (debug_hidden_play_count_ > 0)
            return klondike_.debugHiddenPlay(debug_hidden_play_count_) == false;
     #endif

        int rc = -1;
        for (;;) {
            while ((rc = klondike_.outGame()) < 0) {
                ;
            }
            if (rc == 0)
                break;
            if (rc == 1)
                ++klondike_.opt.seed;
            do {
                klondike_.reset();
                while (klondike_.update()) {
                    ;
                }
            } while (klondike_.result() == 0);
            if (klondike_.result() == 2)
                break;
        }
        klondike_.updateOptions();
        saveIni();
        return 0;
    }

private:

    bool checkOpts(char* arg) {
        if (checkOpt(arg, "-seed")) {
            klondike_.opt.seed      = strtoul(arg, nullptr,10);
        } else if (checkOpt(arg, "-semiauto")) {
            klondike_.opt.semi_auto  = (*arg != '-');
        } else if (checkOpt(arg, "-loop")) {
            klondike_.opt.loop_count = strtoul(arg, nullptr, 0);
        } else if (checkOpt(arg, "-draw3") || checkOpt(arg, "-draw3mode")) {
            klondike_.opt.draw3cards = (*arg != '-');
        } else if (checkOpt(arg, "-hint_always")) {
            klondike_.opt.hint_always= (*arg != '-');
        //} else if (checkOpt(arg, "-auto")) {
        //    klondike_.opt.automode  = (*arg != '-');
        }
     #if KLONDIKE_USE_DIFICULTY_MODE
        else if (checkOpt(arg, "-difficulty") || checkOpt(arg, "-dif")) {
            klondike_.opt.difficulty = strtoul(arg, nullptr, 0);
        } else if (checkOpt(arg, "-debug_hidden_play")) {
            debug_hidden_play_count_ = strtoul(arg, nullptr, 0);
        }
     #endif
        else if (checkOpt(arg, "-debug")) {
            klondike_.opt.debug     = (*arg != '-');
        } else {
            printf("Unkown option : %s\n", arg);
            return false;
        }
        return true;
    }

    bool checkOpt(char*& arg, char const* opt) {
        size_t opt_len = strlen(opt);
        if (strncmp(arg, opt, opt_len) == 0) {
            arg += opt_len;
            if (*arg == '=' || *arg == ':')
                ++arg;
            return true;
        }
        return false;
    }

    void loadDat() {
     #if !defined(USE_CUI) && !defined(__DOS__)
        char fpath[_MAX_PATH];
        getSavedataPath(fpath, sizeof(fpath), ".dat");
        loadIni(fpath);
     #endif
    }

    void loadIni() {
        char fpath[_MAX_PATH];
        getSavedataPath(fpath, sizeof(fpath), ".ini");
        loadIni(fpath);
    }

    void loadIni(char const* fpath) {
        FILE* fp = fopen(fpath, "rb");
        if (fp == NULL)
            return;
        char buf[LOAD_BUF_BYTES+1] = {0};
        size_t  rlen = fread(buf, 1, sizeof(buf)-1, fp);
        fclose(fp);
        unsigned char* b = (unsigned char*)buf;
        unsigned char* e = b + rlen;
        while (b < e) {
            while (*b && *b <= 0x20)
                ++b;
            if (*b == '#') {
                do {
                    ++b;
                } while (*b != '\n');
                continue;
            } else if (*b == 0) {
                break;
            }
            unsigned char* a = b;
            while (*b >= 0x20)
                ++b;
            *b = 0;
            checkOpts((char*)a);
            ++b;
        }
    }

    void saveIni() {
        char fpath[_MAX_PATH];
        getSavedataPath(fpath, sizeof(fpath), ".ini");
        FILE* fp = fopen(fpath, "wb");
        if (!fp)
            return;
        fprintf(fp, "# KLONDIKE\n");
        fprintf(fp, "-seed=%u\n"      , klondike_.opt.seed);
        fprintf(fp, "-difficulty=%u\n", klondike_.opt.difficulty);
        if (klondike_.opt.draw3cards)
            fprintf(fp, "-draw3cards\n");
        if (klondike_.opt.semi_auto)
            fprintf(fp, "-semiauto\n");
        if (klondike_.opt.hint_always)
            fprintf(fp, "-hint_always\n");
        if (klondike_.opt.debug)
            fprintf(fp, "-debug\n");
        //if (klondike_.opt.automode)
        //    fprintf(fp, "-auto\n");
        //fprintf(fp, "-loop=%u\n"    , klondike_.opt.loop_count);
        fclose(fp);
    }

    char* getSavedataPath(char fpath[], size_t sz, char const* ext) {
      #if defined(_WIN32) || defined(__DOS__)
        snprintf(fpath, sz, "%s", appname_);
        if (fpath[0] == 0)
            snprintf(fpath, sz, "klondike");
        char* p = strrchr(fpath, '.');
        if (!p)
            p = fpath + strlen(fpath);
        snprintf(p, fpath+sz-p-1, ext);
      #else
        char const* fname   = ".solitaire_klondike";
        const char *homeDir = getenv("HOME");
        if (!homeDir)
            homeDir = "~";
        snprintf(fpath, sz, "%s/.config", homeDir);
        struct stat st = {0};
        int rc = stat(fpath, &st);
        if (rc == 0 && (st.st_mode & S_IFDIR)) {
            snprintf(fpath, sz, "%s/.config/%s%s", homeDir, fname, ext);
        } else {
            snprintf(fpath, sz, "%s", fname);
        }
      #endif
        return fpath;
    }
};


int main(int argc, char* argv[]) {
    //ConsoleCP   cp;
    int rc = App().main(argc, argv);
    return rc;
}
