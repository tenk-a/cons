// for small compact model
#ifndef IA16_H_INCLUDED
#define IA16_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _REGS_H {
    uint8_t  al, ah;
    uint8_t  bl, bh;
    uint8_t  cl, ch;
    uint8_t  dl, dh;
};

struct _REGS_W {
    uint16_t ax, bx, cx, dx;
    uint16_t si, di;
    uint16_t cflag;
    uint16_t flags;
};

struct _INTR_REGS_W {
    uint16_t ax, bx, cx, dx;
    uint16_t si, di;
    uint16_t cflag;
    uint16_t flags;
    uint16_t es, cs, ss, ds;
    uint16_t bp;
};

typedef struct SREGS {
    uint16_t ds, es, ss, cs;
} SREGS;

typedef union REGS {
    struct _REGS_H h;
    struct _REGS_W w;
    struct _REGS_W x;
} REGS;

typedef union intr_regs_t {
    struct _REGS_H      h;
    struct _INTR_REGS_W w;
    struct _INTR_REGS_W x;
} intr_regs_t;


int int86 (int intr_no, const union REGS* in, union REGS* out);
int int86x(int intr_no, const union REGS* in, union REGS* out, struct SREGS* sr);
int intr  (int intr_no, intr_regs_t* inout);
int _ia16_intrp(int no, intr_regs_t const __far* iregs, intr_regs_t __far* oregs);

static inline uint8_t inp(uint16_t port) {
    uint8_t v; __asm__ __volatile__("inb %1, %0" : "=a"(v) : "dN"(port));
    return v;
}
static inline uint16_t inpw(uint16_t port) {
    uint16_t v; __asm__ __volatile__("inw %1, %0" : "=a"(v) : "dN"(port));
    return v;
}
static inline void outp(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" :: "a"(val), "dN"(port));
}
static inline void outpw(uint16_t port, uint16_t val) {
    __asm__ __volatile__("outw %0, %1" :: "a"(val), "dN"(port));
}

void __far* _fmemset(void __far* dst, int c, size_t bytes);
void __far* _fmemcpy(void __far* dst, void __far* src, size_t bytes);
void __far* _fmalloc(size_t bytes);
void _ffree(void __far* p);

#ifdef __cplusplus
}
#endif
#endif /* IA16_H_INCLUDED */
