cmake_minimum_required(VERSION 3.24)

# cmake:プロジェクト.
project(cons_game C)

# ソース・ディレクトリのパス.
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
# thirdparty ディレクトリのパス.
set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty" CACHE STRING "THIRDPARTY_DIR")

# ツールチェイン名.
get_filename_component(TOOLCHAIN_NAME "${CMAKE_TOOLCHAIN_FILE}" NAME_WE)
string(REPLACE "-toolchain" "" TOOLCHAIN_NAME "${TOOLCHAIN_NAME}")

# mac の場合 CMAKE_TOOLCHAIN_FILE を使わないこともあるので APPLE で判定.
if(APPLE AND NOT TOOLCHAIN_NAME)
  set(TOOLCHAIN_NAME "mac")
endif()

# src/cons 追加.
set(CONS_PLATFORM ${TOOLCHAIN_NAME})
add_subdirectory("${SRC_DIR}/cons")

#       -       -       -       -       -       -       -       -

# cmake:実行ファイル生成
add_executable("mines"
  "${SRC_DIR}/mines/mines.c"

  ${TOOLCHAIN_ADD_SRCS}
)

# cmake:コンパイル・オプション設定.
target_compile_options(     "mines" PRIVATE ${TOOLCHAIN_ADD_OPTS} )

# cmake: include ディレクトリ設定.
target_include_directories( "mines" PRIVATE ${TOOLCHAIN_ADD_INCLUDE_DIRS} ${SRC_DIR} )

# cmake: ライブラリ・ディレクトリ設定.
target_link_directories(    "mines" PRIVATE ${TOOLCHAIN_ADD_LINK_DIRS} )

# cmake:ライブラリ・オプション設定.
target_link_options(        "mines" PRIVATE ${TOOLCHAIN_ADD_LINK_OPTS} )

# cmake: ライブラリ設定.
target_link_libraries(      "mines" PRIVATE cons ${TOOLCHAIN_ADD_LIBS} )

# cmake:インストール先を設定.
install(TARGETS             "mines" RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/bin/${TOOLCHAIN_NAME}" )

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

if(MSVC)
  # VS で開いた時、project() 設定したプロジェクトがカレントになるようにする指定.
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "mines")
endif()

#       -       -       -       -       -       -       -       -
#message(STATUS "TOOLCHAIN_ADD_OPTS: ${TOOLCHAIN_ADD_OPTS}")
#message(STATUS "TOOLCHAIN_ADD_LIBS: ${TOOLCHAIN_ADD_LIBS}")
#message(STATUS "TOOLCHAIN_ADD_LINK_DIRS: ${TOOLCHAIN_ADD_LINK_DIRS}")
#message(STATUS "TOOLCHAIN_ADD_LINK_OPTS: ${TOOLCHAIN_ADD_LINK_OPTS}")
#message(STATUS "TOOLCHAIN_ADD_INCLUDE_DIRS: ${TOOLCHAIN_ADD_INCLUDE_DIRS}")
