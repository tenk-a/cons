cmake_minimum_required(VERSION 3.24)

project(cons C)


if(NOT CONS_PLATFORM)
  get_filename_component(CONS_PLATFORM "${CMAKE_TOOLCHAIN_FILE}" NAME_WE)
  string(REPLACE "-toolchain" "" CONS_PLATFORM "${CONS_PLATFORM}")
  if(APPLE AND NOT CONS_PLATFORM)
    set(CONS_PLATFORM "mac")
  endif()
  if(NOT CONS_PLATFORM)
    message(FARTAL_ERROR "cons: CONS_PLATFORM is not set.")
  endif()
endif()


set(CONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(THIRDPARTY_DIR "${CONS_DIR}/../../thirdparty")
#message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
#message(STATUS "CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}")
#message(STATUS "THIRDPARTY_DIR=${THIRDPARTY_DIR}")
#message(STATUS "CMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}")


set(CONS_SRCS
  "${CONS_DIR}/cons.h"
)
set(CONS_INC_DIRS
  ${CONS_DIR}
)


if(CONS_PLATFORM MATCHES "pc98")
  set(CONS_SRCS "${CONS_SRCS}" "${CONS_DIR}/cons_pc98.h" "${CONS_DIR}/cons_pc98.c")

elseif(CONS_PLATFORM MATCHES "pcat")
  set(CONS_SRCS "${CONS_SRCS}" "${CONS_DIR}/cons_pcat.h" "${CONS_DIR}/cons_pcat.c")

elseif(CONS_PLATFORM MATCHES "mac" OR CONS_PLATFORM MATCHES "linux")
  list(APPEND CONS_LIBS "ncurses")
  set(CONS_SRCS "${CONS_SRCS}" "${CONS_DIR}/cons_curses.h" "${CONS_DIR}/cons_curses.c")

elseif(CONS_PLATFORM MATCHES "win" OR CONS_PLATFORM MATCHES "dos")
  list(APPEND CONS_OPTS     "-DCONS_USE_PDCURSES")
  list(APPEND CONS_INC_DIRS "${THIRDPARTY_DIR}/include")
  list(APPEND CONS_LIB_DIRS "${THIRDPARTY_DIR}/lib/${CONS_PLATFORM}")
  list(APPEND CONS_LIBS     "pdcurses")
  set(CONS_SRCS "${CONS_SRCS}" "${CONS_DIR}/cons_curses.h" "${CONS_DIR}/cons_curses.c")
endif()


add_library(cons OBJECT
    ${CONS_SRCS}
)
target_compile_options(cons PUBLIC
  ${CONS_OPTS}
)
target_include_directories(cons PUBLIC
  ${CONS_INC_DIRS}
)
target_link_directories(cons PUBLIC
  ${CONS_LIB_DIRS}
)
target_link_libraries(cons PUBLIC
  ${CONS_LIBS}
)
