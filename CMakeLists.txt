cmake_minimum_required(VERSION 3.24)

# cmake:プロジェクト.
project(otige C)

# ソース・ディレクトリのパス.
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
# thirdparty ディレクトリのパス.
set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

# ソース・ファイル.
set(SRCS
  ${SRC_DIR}/otige.c
  ${SRC_DIR}/cons.h
)
# include ディレクトリ.
set(INC_DIRS
  ${SRC_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)
# コンパイラ・オプション.
set(OPTS )
# ライブラリ・ディレクトリ.
set(LIB_DIRS )
# ライブラリ・ファイル.
set(LIBS )

# ツールチェイン名.
get_filename_component(TOOLCHAIN_NAME "${CMAKE_TOOLCHAIN_FILE}" NAME_WE)
string(REPLACE "-toolchain" "" TOOLCHAIN_NAME "${TOOLCHAIN_NAME}")
if(APPLE AND NOT TOOLCHAIN_NAME)
  set(TOOLCHAIN_NAME "mac")
endif()

# コンパイラ・OS環境別の設定.
if(APPLE OR TOOLCHAIN_NAME MATCHES "mac" OR TOOLCHAIN_NAME MATCHES "linux")
  # mac linux(unix) では ncurses を使う.
  list(APPEND LIBS "ncurses")
elseif(TOOLCHAIN_NAME MATCHES "win" OR TOOLCHAIN_NAME MATCHES "dos")
  # Win / Dos なら PDCurses を使うための設定.
  list(APPEND LIB_DIRS "${THIRDPARTY_DIR}/lib/${TOOLCHAIN_NAME}")
  list(APPEND INC_DIRS "${THIRDPARTY_DIR}/include")
  list(APPEND LIBS "pdcurses")
  list(APPEND OPTS "-DUSE_PDCURSES")

  # VS で開いた時、project() 設定したプロジェクトがカレントになるようにする指定.
  if(TOOLCHAIN_NAME MATCHES "vc")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
  endif()
endif()

# cmake:実行ファイル生成
add_executable(${PROJECT_NAME}
  ${SRCS}
  ${TOOLCHAIN_ADD_SRCS}
)
# cmake:インストール先を設定.
install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/bin/${TOOLCHAIN_NAME}"
)
# cmake:コンパイル・オプション設定.
target_compile_options(${PROJECT_NAME} PRIVATE
  ${TOOLCHAIN_ADD_OPTS}
  ${OPTS}
)
# cmake: include ディレクトリ設定.
target_include_directories(${PROJECT_NAME} PRIVATE
  ${TOOLCHAIN_ADD_INCLUDE_DIRS}
  ${INC_DIRS}
)
# cmake: ライブラリ・ディレクトリ設定.
target_link_directories(${PROJECT_NAME} PRIVATE
  ${LIB_DIRS}
  ${TOOLCHAIN_ADD_LINK_DIRS}
)
# cmake: ライブラリ設定.
target_link_libraries(${PROJECT_NAME} PRIVATE
  ${LIBS}
  ${TOOLCHAIN_ADD_LIBS}
)
