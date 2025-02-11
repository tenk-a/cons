// カーソル移動できる Hello world! 表示. ESC か Q で終了.
#if defined(USE_PDCURSES)
#include <curses.h>
#else
#include <ncurses.h>
#endif

int main(void) {
    // 初期化.
    enum { N = 12};                                     // Hello world! 文字数.
    int  x, y, w, h, count = 0;
    initscr();                                          // curses:(スクリーン)初期化.
    noecho();                                           // curses:キー入力で表示を行わない.
    cbreak();                                           // curses:入力バッファリングしない.
    keypad(stdscr, TRUE);                               // curses:カーソルキーを有効にする.
    curs_set(0);                                        // curses:カーソルを表示しない.
    timeout(50);                                        // curses:50ミリ秒でgetchをtimeoutさせる指定.
    getmaxyx(stdscr, h, w);                             // curses:画面サイズ取得.
    x = (w - N) / 2;                                    // x初期位置:画面中央.
    y = (h - 1) / 2;                                    // y初期位置:画面中央.

    // 20FPS風ループ.
    for (;;) {
        int k = getch();                                // curses:1文字入力. 50ミリ秒でtimeout.
        if (k == 0x1b || k == 'q' || k == 'Q')          // ESCまたは Q キーで終了.
            break;
        x = x - (k == KEY_LEFT) + (k == KEY_RIGHT);     // 左右カーソルキーで増減.
        y = y - (k == KEY_UP  ) + (k == KEY_DOWN );     // 上下カーソルキーで増減.
        x = (x < 0) ? 0 : (x > w - N) ? (w - N) : x;    // x移動範囲チェック.
        y = (y < 0) ? 0 : (y > h - 1) ? (h - 1) : y;    // y移動範囲チェック.

        erase();                                        // curses:画面バッファ クリア.
        move(y, x);                                     // curses:表示位置設定.
        if (count & 0x0c)                               // フレーム数をみて点滅させる.
            addstr("Hello world!");                     // curses:Hello world! 表示.
        ++count;                                        // フレームカウンタ更新.
        refresh();                                      // curses:画面バッファを実画面に反映.
    }
    // 終了.
    endwin();                                           // curses 終了.
    return 0;
}
