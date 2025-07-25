// カーソル移動できる Hello world! 表示. ESC か Q で終了.
#include <stdio.h>
#include "../cons/cons.h"

int main(void) {
    int x, y, w, h, k, count = 0;
    enum { N = 12};                                             // Hello world! 文字数.
    cons_init(0);
    w = cons_screenWidth();
    h = cons_screenHeight();
    x = (w - N) / 2;
    y = (h - 1) / 2;

    for (;;) {
        cons_updateBegin();
        k = cons_key();                                         // curses:1文字入力.
        if (k == 0x1b || k == 'q' || k == 'Q')                  // ESCまたは Q キーで終了.
            break;
        x = x - (k == CONS_KEY_LEFT) + (k == CONS_KEY_RIGHT);   // 左右カーソルキーで増減.
        y = y - (k == CONS_KEY_UP  ) + (k == CONS_KEY_DOWN );   // 上下カーソルキーで増減.
        x = (x < 0) ? 0 : (x > w - N) ? (w - N) : x;            // x移動範囲チェック.
        y = (y < 0) ? 0 : (y > h - 1) ? (h - 1) : y;            // y移動範囲チェック.

        cons_clear();                                           // curses:画面バッファ クリア.
        if (count & 0x0c)                                       // フレーム数をみて点滅させる.
            cons_xyprintf(x, y, "Hello world!");                // curses:Hello world! 表示.
        ++count;                                                // フレームカウンタ更新.
        cons_updateEnd();
    }
    // 終了.
    cons_term();                                                // cons 終了.
    return 0;
}
