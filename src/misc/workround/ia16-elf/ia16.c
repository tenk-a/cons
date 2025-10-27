#include "ia16.h"
#include "../../../cons/dbg.h"


void __far* _fmemset(void __far* dst, int c, size_t bytes) {
    __asm__ __volatile__ (
        "push %%ds\n\t"
        "push %%es\n\t"
        "mov %[n], %%cx\n\t"
        "mov %[val], %%al\n\t"
        "les %[dst], %%di\n\t"
        "rep stosb\n\t"
        "pop %%es\n\t"
        "pop %%ds\n\t"
        :
        : [dst]"m"(dst), [val]"r"((uint8_t)c), [n]"r"(bytes)
        : "cc","memory","al","cx","di","es","ds"
    );
    return dst;
}

void __far* _fmemcpy(void __far* dst, void __far* src, size_t bytes) {
    __asm__ __volatile__ (
        "push %%ds\n\t"
        "push %%es\n\t"
        "mov %[n], %%cx\n\t"
        "les %[dst], %%di\n\t"
        "lds %[src], %%si\n\t"
        "rep movsb\n\t"
        "pop %%es\n\t"
        "pop %%ds\n\t"
        :
        : [dst]"m"(dst), [src]"m"(src), [n]"r"(bytes)
        : "cc","memory","cx","si","di","es","ds"
    );
    return dst;
}


int intr(int intr_no, intr_regs_t* inout) {
    return _ia16_intrp(intr_no, inout, inout);
}

int int86(int intr_no, const union REGS* inr, union REGS* outr) {
    intr_regs_t iregs;
    intr_regs_t oregs;
    *(union REGS*)&iregs = *inr;
    iregs.w.es = iregs.w.cs = iregs.w.ss = iregs.w.ds = iregs.w.bp = 0;
    _ia16_intrp(intr_no, &iregs, &oregs);
    *outr = *(union REGS const*)&oregs;
    return outr->w.cflag;
}

int int86x(int intr_no, const union REGS* inr, union REGS* outr, struct SREGS* sregs) {
    intr_regs_t iregs;
    intr_regs_t oregs;
    *(union REGS*)&iregs = *inr;
    *(struct SREGS*)&iregs.w.ds = *sregs;
    iregs.w.bp = 0;
    _ia16_intrp(intr_no, &iregs, &oregs);
    *sregs = *(struct SREGS*)&iregs.w.ds;
    *outr  = *(union REGS const*)&oregs;
    return outr->w.cflag;
}

void __far* _fmalloc(size_t bytes) {
    intr_regs_t r = { 0 };
    intr_regs_t r2 = { 0 };
    r.w.ax = 0x4800;
    r.w.bx = (uint16_t)((bytes + 15u) >> 4);
    if (_ia16_intrp(0x21, &r, &r2) == 0) {
        uint32_t fp = ((uint32_t)r2.w.ax << 16) | 0x0000u;
        return (void __far*)(fp);
    }
    return (void __far*)0;
}

void _ffree(void __far* p) {
    intr_regs_t r = {0};
    r.w.ax = 0x4900;
    r.w.es = (uint32_t)p >> 16;
    (void)intr(0x21, &r);
}
