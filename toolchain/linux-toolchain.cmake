#set(TOOLCHAIN_NAME "linux" CACHE STRING "Toolchain name")

add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8 -fwide-exec-charset=utf-32LE)
add_compile_options(-ffunction-sections -fdata-sections)

set(CMAKE_EXE_LINKER_FLAGS_RELEASE        "-Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL     "-Wl,--gc-sections")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")
endif()
