cmake_minimum_required(VERSION 3.24)

project(cons_game C CXX ASM)

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty" CACHE STRING "THIRDPARTY_DIR")

# Get toolchain name.
get_filename_component(TOOLCHAIN_NAME "${CMAKE_TOOLCHAIN_FILE}" NAME_WE)
string(REPLACE "-toolchain" "" TOOLCHAIN_NAME "${TOOLCHAIN_NAME}")

if(APPLE AND NOT TOOLCHAIN_NAME)
  set(TOOLCHAIN_NAME "mac")
endif()

# src/cons.
set(CONS_PLATFORM ${TOOLCHAIN_NAME})
add_subdirectory("${SRC_DIR}/cons")

#       -       -       -       -       -       -       -       -

add_executable("mines"
  "${SRC_DIR}/mines/mines.c"

  ${TOOLCHAIN_ADD_SRCS}
)
target_compile_options(     "mines" PRIVATE ${TOOLCHAIN_ADD_OPTS} )
target_include_directories( "mines" PRIVATE ${TOOLCHAIN_ADD_INCLUDE_DIRS} ${SRC_DIR} )
target_link_directories(    "mines" PRIVATE ${TOOLCHAIN_ADD_LINK_DIRS} )
target_link_options(        "mines" PRIVATE ${TOOLCHAIN_ADD_LINK_OPTS} )
target_link_libraries(      "mines" PRIVATE cons ${TOOLCHAIN_ADD_LIBS} )
install(TARGETS             "mines" RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/bin/${TOOLCHAIN_NAME}" )

#       -       -       -       -       -       -       -       -

add_executable("hello"
  "${SRC_DIR}/hello/hello.c"

  ${TOOLCHAIN_ADD_SRCS}
)
target_compile_options(    "hello" PRIVATE ${TOOLCHAIN_ADD_OPTS} )
target_include_directories("hello" PRIVATE ${TOOLCHAIN_ADD_INCLUDE_DIRS} ${SRC_DIR} )
target_link_directories(   "hello" PRIVATE ${TOOLCHAIN_ADD_LINK_DIRS} )
target_link_options(       "hello" PRIVATE ${TOOLCHAIN_ADD_LINK_OPTS} )
target_link_libraries(     "hello" PRIVATE cons ${TOOLCHAIN_ADD_LIBS} )
install(TARGETS            "hello" RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/bin/${TOOLCHAIN_NAME}")

#       -       -       -       -       -       -       -       -

add_executable("otitame"
  "${SRC_DIR}/otitame/otitame.c"

  ${TOOLCHAIN_ADD_SRCS}
)
target_compile_options(    "otitame" PRIVATE ${TOOLCHAIN_ADD_OPTS} )
target_include_directories("otitame" PRIVATE ${TOOLCHAIN_ADD_INCLUDE_DIRS} ${SRC_DIR} )
target_link_directories(   "otitame" PRIVATE ${TOOLCHAIN_ADD_LINK_DIRS} )
target_link_options(       "otitame" PRIVATE ${TOOLCHAIN_ADD_LINK_OPTS} )
target_link_libraries(     "otitame" PRIVATE cons ${TOOLCHAIN_ADD_LIBS} )
install(TARGETS            "otitame" RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/bin/${TOOLCHAIN_NAME}")

#       -       -       -       -       -       -       -       -

if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "ia16" AND NOT CMAKE_TOOLCHAIN_FILE MATCHES "watcom-dos16-s")

add_executable("klondike"
  "${SRC_DIR}/klondike/main.cpp"
  "${SRC_DIR}/klondike/Klondike.cpp"
  "${SRC_DIR}/klondike/Klondike.hpp"
  "${SRC_DIR}/klondike/KlondikeTuiView.cpp"
  "${SRC_DIR}/klondike/KlondikeTuiView.hpp"
  "${SRC_DIR}/cons/ConScr.hpp"
  "${SRC_DIR}/misc/static_inttype_vector.hpp"

  ${TOOLCHAIN_ADD_SRCS}
)
target_compile_options(    "klondike" PRIVATE ${TOOLCHAIN_ADD_OPTS} )
target_include_directories("klondike" PRIVATE ${TOOLCHAIN_ADD_INCLUDE_DIRS} ${SRC_DIR} )
target_link_directories(   "klondike" PRIVATE ${TOOLCHAIN_ADD_LINK_DIRS} )
target_link_options(       "klondike" PRIVATE ${TOOLCHAIN_ADD_LINK_OPTS} )
target_link_libraries(     "klondike" PRIVATE cons ${TOOLCHAIN_ADD_LIBS} )
install(TARGETS            "klondike" RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/bin/${TOOLCHAIN_NAME}")

endif()

#       -       -       -       -       -       -       -       -

if(NOT CMAKE_TOOLCHAIN_FILE MATCHES "ia16")

add_executable("klon_cui"
  "${SRC_DIR}/klondike/main.cpp"
  "${SRC_DIR}/klondike/Klondike.cpp"
  "${SRC_DIR}/klondike/Klondike.hpp"
  "${SRC_DIR}/klondike/KlondikeConsoleView.hpp"
  "${SRC_DIR}/misc/static_inttype_vector.hpp"

  ${TOOLCHAIN_ADD_SRCS}
)
target_compile_options(    "klon_cui" PRIVATE "-DUSE_CUI" ${TOOLCHAIN_ADD_OPTS} )
target_include_directories("klon_cui" PRIVATE ${TOOLCHAIN_ADD_INCLUDE_DIRS} ${SRC_DIR} )
target_link_directories(   "klon_cui" PRIVATE ${TOOLCHAIN_ADD_LINK_DIRS} )
target_link_options(       "klon_cui" PRIVATE ${TOOLCHAIN_ADD_LINK_OPTS} )
target_link_libraries(     "klon_cui" PRIVATE ${TOOLCHAIN_ADD_LIBS} )
install(TARGETS            "klon_cui" RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/bin/${TOOLCHAIN_NAME}")

endif()

#       -       -       -       -       -       -       -       -

if(MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "mines")
endif()

#       -       -       -       -       -       -       -       -
#message(STATUS "TOOLCHAIN_ADD_OPTS: ${TOOLCHAIN_ADD_OPTS}")
#message(STATUS "TOOLCHAIN_ADD_LIBS: ${TOOLCHAIN_ADD_LIBS}")
#message(STATUS "TOOLCHAIN_ADD_LINK_DIRS: ${TOOLCHAIN_ADD_LINK_DIRS}")
#message(STATUS "TOOLCHAIN_ADD_LINK_OPTS: ${TOOLCHAIN_ADD_LINK_OPTS}")
#message(STATUS "TOOLCHAIN_ADD_INCLUDE_DIRS: ${TOOLCHAIN_ADD_INCLUDE_DIRS}")
