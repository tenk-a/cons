/**
 *  @file cons.h
 *  @brief A console screen library header.
 *  @author Masashi Kitamura ( https://github.com/tenk-a/ )
 *  @date   2024-12
 *  @license Boost Software License - Version 1.0
 */
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
