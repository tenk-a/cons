/**
 *  @file dos_wrap.h
 *  @brief dos define wrapper
 *  @date   2025-08
 *  @license Boost Software License - Version 1.0
 */
#ifndef DOS_WRAP_H_
#define DOS_WRAP_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

#if defined(__WATCOMC__)
 #if defined(__PC98__)
  #define __WATCOM_PC98__
 #endif
 #include <bios.h>
 #include <i86.h>
 #include <io.h>
 #include <malloc.h>
 #include <dos.h>
 #include <conio.h>
#elif defined(__DJGPP__)
 #ifndef __FLAT__
  #define __FLAT__
 #endif
 #include <dpmi.h>
 #include <go32.h>
 #include <sys/nearptr.h>
 #include <sys/movedata.h>
 #include <unistd.h>
 #include <malloc.h>
 #include <dos.h>
 #include <conio.h>
#else //__IA16__
 #include <unistd.h>
 #include <ia16.h>
#endif


#if defined(__FLAT__) == 0  // 16bit DOS
 #define FAR                __far
 #define MK_FAR_PTR(a,b)    (((uint32_t)(a) << 16) | (b))
 #define FAR_PTR_SEG(p)     ((uint16_t)((uint32_t)(void FAR*)(p) >> 16))
 #define FAR_PTR_OFF(p)     ((uint16_t)((uint32_t)(p)))
 #define FAR_MALLOC         _fmalloc
 #define FAR_FREE           _ffree
 #define FAR_MEMCPY         _fmemcpy
 #define FAR_MEMSET         _fmemset
 #define FAR_ALIGN_PTR(t,p,a)  ((t)(((uint32_t)(p) + (a) - 1) & ~((a) - 1)))
 #define DOS_PEEKB(ofs)     (*(uint8_t  volatile __far*)(ofs))
 #define DOS_PEEKW(ofs)     (*(uint16_t volatile __far*)(ofs))
 #define DOS_PEEKD(ofs)     (*(uint32_t volatile __far*)(ofs))
 #define DOS_POKEB(ofs,b)   (*(uint8_t  volatile __far*)(ofs) = (b))
 #define DOS_POKEW(ofs,w)   (*(uint16_t volatile __far*)(ofs) = (w))
 #define DOS_POKED(ofs,d)   (*(uint32_t volatile __far*)(ofs) = (d))
 #define DOS_MEMPUT(srcptr,bytes,dosadr)    _fmemcpy((void __far*)(dosadr), (srcptr), (bytes))
 #define DOS_MEMGET(dosadr,bytes,dstptr)    _fmemcpy(dstptr, (void __far*)(dosadr), (bytes))
 #define DOS_ADDR_TO(x)     ((uint8_t FAR*)(x))
 #define DOS_ADDR_FROM(x)   ((uint32_t)(x))
 #define DOS_ADDR_INIT()    (1)
 #define DOS_ADDR_TERM()    ((void)(0))
 #if defined(__WATCOMC__)
  #define _W                 w
  #define INTR_REGS          union REGPACK
  #define INTR(n,r)          intr((n),(r))
  typedef void (_interrupt __far *DOS_VECT_ADR)();
  #define DOS_CLEAR_VECT_ADR(v) ((v) = 0)
  #define DOS_GETVECT(n)     _dos_getvect(n)
  #define DOS_RESETVECT(n,h) _dos_setvect((n),(h))
  #define DOS_SETVECT(n,h)   _dos_setvect((n),(h))
 #elif defined(__ia16__)
  #define _W                 w
  #define INTR_REGS          intr_regs_t
  #define INTR(n,r)          intr((n),(r))
 #endif
 #undef  __loadds
 #define __loadds
#elif defined(__FLAT__)   // 32bit DOS
 #define FAR
 #define MK_FAR_PTR(a,b)    (((a) << 4) | (b))
 #define FAR_PTR_SEG(p)     ((uint16_t)((size_t)(p) >> 4))
 #define FAR_PTR_OFF(p)     ((uint16_t)((size_t)(p) &  3))
 #define FAR_MALLOC         malloc
 #define FAR_FREE           free
 #define FAR_MEMCPY         memcpy
 #define FAR_MEMSET         memset
 #define FAR_ALIGN_PTR(t,p,a)  ((t)(((uintptr_t)(p) + (a) - 1) & ~((a) - 1)))
 #if defined(__WATCOMC__)
  #define DOS_PEEKB(ofs)    (*(uint8_t  volatile*)(ofs))
  #define DOS_PEEKW(ofs)    (*(uint16_t volatile*)(ofs))
  #define DOS_PEEKD(ofs)    (*(uint32_t volatile*)(ofs))
  #define DOS_POKEB(ofs,b)  (*(uint8_t  volatile*)(ofs) = (b))
  #define DOS_POKEW(ofs,w)  (*(uint16_t volatile*)(ofs) = (w))
  #define DOS_POKED(ofs,d)  (*(uint32_t volatile*)(ofs) = (d))
  #define DOS_MEMPUT(srcptr,bytes,dosadr)   memcpy((void*)(dosadr), (srcptr), (bytes))
  #define DOS_MEMGET(dosadr,bytes,dstptr)   memcpy((dstptr), (void*)(dosadr), (bytes))
  #define _W                w
  #define INTR_REGS         union REGPACK
  #define INTR(n,r)         intrf((n),(r))
  #define int86             int386
  #define int86x            int386x
  //extern uint8_t          __isPC98;          // watcom
  typedef void (_interrupt __far *DOS_VECT_ADR)();
  #define DOS_CLEAR_VECT_ADR(v) ((v) = 0)
  #define DOS_GETVECT(n)     _dos_getvect(n)
  #define DOS_RESETVECT(n,h) _dos_setvect((n),(h))
  #define DOS_SETVECT(n,h)   _dos_setvect((n),(h))
  #define DOS_ADDR_TO(x)     ((uint8_t*)(x))
  #define DOS_ADDR_FROM(x)   ((uint32_t)(x))
  #define DOS_ADDR_INIT()    (1)
  #define DOS_ADDR_TERM()    ((void)(0))
 #elif defined(__DJGPP__)
  #define __far
  #define DOS_PEEKB(ofs)    _far_peek_b((uint32_t)(ofs))
  #define DOS_PEEKW(ofs)    _far_peek_w((uint32_t)(ofs))
  #define DOS_PEEKD(ofs)    _far_peek_d((uint32_t)(ofs))
  #define DOS_POKEB(ofs,b)  _far_poke_b((uint32_t)(ofs), (b))
  #define DOS_POKEW(ofs,w)  _far_poke_w((uint32_t)(ofs), (w))
  #define DOS_POKED(ofs,d)  _far_poke_d((uint32_t)(ofs), (d))
  static inline uint8_t     _far_peek_b(uint32_t dosptr) { uint8_t  b; dosmemget(dosptr, sizeof(b), &b); return b; }
  static inline uint16_t    _far_peek_w(uint32_t dosptr) { uint16_t w; dosmemget(dosptr, sizeof(w), &w); return w; }
  static inline uint32_t    _far_peek_d(uint32_t dosptr) { uint32_t d; dosmemget(dosptr, sizeof(d), &d); return d; }
  static inline void        _far_poke_b(uint32_t dosptr, uint8_t  b) { dosmemput(&b, sizeof(b), dosptr); }
  static inline void        _far_poke_w(uint32_t dosptr, uint16_t w) { dosmemput(&w, sizeof(w), dosptr); }
  static inline void        _far_poke_d(uint32_t dosptr, uint32_t d) { dosmemput(&d, sizeof(d), dosptr); }
  #define DOS_MEMPUT(srcptr,bytes,dosadr)   dosmemput((srcptr), (bytes), (dosadr))
  #define DOS_MEMGET(dosadr,bytes,dstptr)   dosmemget((dosadr), (bytes), (dstptr))
  #define _W                x
  #define INTR_REGS         __dpmi_regs
  #define INTR(n,r)         __dpmi_int((n),(r))
  #define DOS_ADDR_TO(x)    ((uint8_t*)(__djgpp_conventional_base + (x)))
  #define DOS_ADDR_FROM(x)  ((uint32_t)(x) -  (uint32_t)__djgpp_conventional_base)
  #define DOS_ADDR_INIT()   (__djgpp_nearptr_enable())
  #define DOS_ADDR_TERM()   (__djgpp_nearptr_disable())
  #if 0 // sippai chu
  typedef __dpmi_paddr      DOS_VECT_ADR;
  #define DOS_CLEAR_VECT_ADR(vi) ((vi).offset32 = 0, (vi).selector = 0)
  static inline DOS_VECT_ADR DOS_GETVECT(uint8_t vec) {
    __dpmi_paddr vect = {0,0};
    __dpmi_get_protected_mode_interrupt_vector(vec, &vect);
    return vect;
  }
  static inline int DOS_RESETVECT(uint8_t vec, DOS_VECT_ADR vect) {
    if (vect.selector | vect.offset32)
        return __dpmi_set_protected_mode_interrupt_vector(vec, &vect);
    return -1;
  }
  static inline int DOS_SETVECT(uint8_t vec, void (*handler)(void)) {
    __dpmi_paddr vect = { (uintptr_t)handler, _go32_my_cs() };
    return __dpmi_set_protected_mode_interrupt_vector(vec, &vect);
  }
  #endif
 #endif
#endif


/*memo:
  watcom djgpp 共通 ざっくり同じ 似通ったもの

    int86,int86x (int386,int386x)
    inp,inpw,inpd,outp,outpw,outpd

  dos.h
    _dos_getdate, _dos_setdate, _dos_gettime, _dos_settime, _dos_setftime
    _dos_creat, _dos_creatnew, _dos_open, _dos_write, _dos_read, _dos_close
    _dos_commit, _dos_findfirst, _dos_findnext, _dos_getftime, _dos_setftime
    _dos_getfileattr, _dos_setfileattr, _dos_getdrive, _dos_setdrive
    _dos_getdiskfree, _dosexterr, _dostrerr

  conio.h
    kbhit, getch, getche, ungetch, putch
    cgets, cputs, cprintf, cscanf

    および その関係の型＆定数.
 */
#endif
