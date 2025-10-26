#include "../../cons/dos_wrap.h"
#include <stdio.h>
#include <string.h>

static int isDosv(void) {
    union  REGS  r = {0};
    struct SREGS s = {0};
    r._W.ax = 0x5000;
    int86x(0x15, &r, &r, &s);
    if ((s.es | r._W.di))
        return 1;
    s.es = 0;
    r._W.ax = 0xFE00;
    int86x(0x10, &r, &r, &s);
    return (s.es != 0xB800 && s.es != 0xB000 && s.es != 0x0000);
}

int main(void) {
    if (isDosv()) {
        puts("DOS/V environment detected.");
    } else {
        puts("Not DOS/V (or English mode without DOS/V extensions).");
    }
    return 0;
}
