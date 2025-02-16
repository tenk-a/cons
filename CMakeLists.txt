cmake_minimum_required(VERSION 3.24)

# cmake:プロジェクト.
set(PROJ_NAME1 mines)
project(${PROJ_NAME1} C)

# VS で開いた時、project() 設定したプロジェクトがカレントになるようにする指定.
if(MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJ_NAME1})
endif()

# ソース・ディレクトリのパス.
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
# thirdparty ディレクトリのパス.
set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

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

#	-	-	-	-	-	-	-	-

# cmake:実行ファイル生成
add_executable(${PROJ_NAME1}
  "${SRC_DIR}/mines/mines.c"
  ${TOOLCHAIN_ADD_SRCS}
)

# cmake:コンパイル・オプション設定.
target_compile_options(${PROJ_NAME1} PRIVATE
  ${TOOLCHAIN_ADD_OPTS}
)

# cmake: include ディレクトリ設定.
target_include_directories(${PROJ_NAME1} PRIVATE
  ${TOOLCHAIN_ADD_INCLUDE_DIRS}
  ${SRC_DIR}
)

# cmake: ライブラリ・ディレクトリ設定.
target_link_directories(${PROJ_NAME1} PRIVATE
  ${TOOLCHAIN_ADD_LINK_DIRS}
)

# cmake: ライブラリ設定.
target_link_libraries(${PROJ_NAME1} PRIVATE
  cons
  ${TOOLCHAIN_ADD_LIBS}
)

# cmake:インストール先を設定.
install(TARGETS ${PROJ_NAME1}
  RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/bin/${TOOLCHAIN_NAME}"
)

#	-	-	-	-	-	-	-	-
set(PROJ_NAME2 otitame)
project(${PROJ_NAME2} C)

# cmake:実行ファイル生成
add_executable(${PROJ_NAME2}
  "${SRC_DIR}/otitame/otitame.c"
  ${TOOLCHAIN_ADD_SRCS}
)

# cmake:コンパイル・オプション設定.
target_compile_options(${PROJ_NAME2} PRIVATE
  ${TOOLCHAIN_ADD_OPTS}
)

# cmake: include ディレクトリ設定.
target_include_directories(${PROJ_NAME2} PRIVATE
  ${TOOLCHAIN_ADD_INCLUDE_DIRS}
  ${SRC_DIR}
)

# cmake: ライブラリ・ディレクトリ設定.
target_link_directories(${PROJ_NAME2} PRIVATE
  ${TOOLCHAIN_ADD_LINK_DIRS}
)

# cmake: ライブラリ設定.
target_link_libraries(${PROJ_NAME2} PRIVATE
  cons
  ${TOOLCHAIN_ADD_LIBS}
)

# cmake:インストール先を設定.
install(TARGETS ${PROJ_NAME2}
  RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/bin/${TOOLCHAIN_NAME}"
)
