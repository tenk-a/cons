# toolchain
set(CMAKE_C_COMPILER ia16-elf-gcc)
set(CMAKE_CXX_COMPILER ia16-elf-g++)
set(CMAKE_AR ia16-elf-ar CACHE FILEPATH "Arhiver")
set(CMAKE_RANLIB ia16-elf-ranlib CACHE FILEPATH "Ranlib")
set(CMAKE_AS ia16-elf-as)
set(CMAKE_NM ia16-elf-nm)
set(CMAKE_OBJDUMP ia16-elf-objdump)

# to pass compiler test
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# target
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR I86)

set(CMAKE_SYSROOT "/usr/ia16-elf")
set(CMAKE_INCLUDE_PATH "${CMAKE_SYSROOT}/include")
set(CMAKE_LIBRARY_PATH "${CMAKE_SYSROOT}/lib")
set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# flags
set(CMAKE_C_FLAGS   "-fno-stack-protector -static -march=i8086 -mcmodel=small -fomit-frame-pointer ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-fno-stack-protector -static -march=i8086 -mcmodel=small -fomit-frame-pointer ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-fnothrow-opt -fdelete-dead-exceptions -fno-rtti -fno-exceptions ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE   "-Os -DNDEBUG" CACHE STRING "CMAKE_C_FLAGS_RELEASE")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG" CACHE STRING "CMAKE_CXX_FLAGS_RELEASE")

set(CMAKE_C_FLAGS   "-D__DOS__ -D__PCAT__ ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-D__DOS__ -D__PCAT__ ${CMAKE_CXX_FLAGS}")

# Extension
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
set(CMAKE_EXECUTABLE_SUFFIX_ASM ".exe")
set(CMAKE_EXECUTABLE_SUFFIX_C ".exe")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".exe")

# library
#set(IA16_ELF_HAS_I86_LIB 1)
if(IA16_ELF_HAS_I86_LIB)
  set(TOOLCHAIN_ADD_LIBS "i86" "dos-s" CACHE STRING "Add libraries.")
else()
  set(ADD_OPTS "-I${CMAKE_CURRENT_LIST_DIR}/../src/misc/workround/ia16-elf/ ${ADD_OPTS}")
  set(TOOLCHAIN_ADD_SRCS "${CMAKE_CURRENT_LIST_DIR}/../src/misc/workround/ia16-elf/ia16.c;${CMAKE_CURRENT_LIST_DIR}/../src/misc/workround/ia16-elf/ia16_intrp.s" CACHE STRING "TOOLCHAIN_ADD_SRCS")
endif()
set(CMAKE_C_FLAGS   "${ADD_OPTS} ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${ADD_OPTS} ${CMAKE_CXX_FLAGS}")
