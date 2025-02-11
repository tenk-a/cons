#set(TOOLCHAIN_NAME "mingw-win64" CACHE STRING "Toolchain name")

add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8 -fwide-exec-charset=utf-32LE)

# Use Windows UTF-8 API (windows10 1903 or later)
set(TOOLCHAIN_ADD_SRCS "${CMAKE_CURRENT_LIST_DIR}/../src/win/ActiveCodePageUTF8.rc")

add_compile_options(-DCONS_USE_UNICODE)
