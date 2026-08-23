#set(TOOLCHAIN_NAME "mingw-win64" CACHE STRING "Toolchain name")

add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8 -fwide-exec-charset=utf-32LE)
add_compile_options(-ffunction-sections -fdata-sections)

set(CMAKE_EXE_LINKER_FLAGS_RELEASE        "-Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL     "-Wl,--gc-sections")

# Use Windows UTF-8 API (windows10 1903 or later)
set(TOOLCHAIN_ADD_SRCS "${CMAKE_CURRENT_LIST_DIR}/../src/win/ActiveCodePageUTF8.rc")

add_compile_options(-DCONS_USE_UNICODE)

#set(TOOLCHAIN_ADD_LIBS "winmm" CACHE STRING "Windows libraries")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")
endif()
