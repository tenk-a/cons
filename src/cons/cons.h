#ifndef CONS_H__
#define CONS_H__

#if defined(__PC98__)
  #include "cons_pc98.h"
#elif defined(__PCAT__)
  #include "cons_pcat.h"
#else
 #include "cons_curses.h"
#endif

#endif //CONS_H__
