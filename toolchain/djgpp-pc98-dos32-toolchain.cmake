#set(TOOLCHAIN_NAME "djgpp-pc98-dos32" CACHE STRING "Toolchain name")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86)
set(CMAKE_C_COMPILER i386-pc-msdosdjgpp-gcc)
set(CMAKE_CXX_COMPILER i386-pc-msdosdjgpp-g++)

set(CMAKE_C_FLAGS "-D__FLAT__ -D__DOS__ -D__PC98__ -march=i386 -mtune=i386 -msoft-float -mno-fp-ret-in-387 -fno-tree-vectorize")
set(CMAKE_CXX_FLAGS "-D__FLAT__ -D__DOS__ -D__PC98__ -march=i386 -mtune=i386 -msoft-float -mno-fp-ret-in-387 -fno-tree-vectorize")

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -lemu")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

set(CMAKE_EXECUTABLE_SUFFIX ".exe")
set(CMAKE_EXECUTABLE_SUFFIX_ASM ".exe")
set(CMAKE_EXECUTABLE_SUFFIX_C ".exe")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".exe")
