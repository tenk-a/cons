#set(TOOLCHAIN_NAME "watcom-pc98-dos16-l" CACHE STRING "Toolchain name")

set(CMAKE_SYSTEM_NAME "dos")
set(CMAKE_SYSTEM_PROCESSOR "I86")
set(CMAKE_C_COMPILER "wcl")
set(CMAKE_CXX_COMPILER "wcl")

set(CMAKE_WATCOM_RUNTIME_LIBRARY "SingleThreaded")

set(CMAKE_C_FLAGS   "-ml -k5000 -D__DOS__ -D__PC98__ ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-ml -k5000 -D__DOS__ -D__PC98__ ${CMAKE_CXX_FLAGS}")

set(TOOLCHAIN_ADD_LIBS "pc98l" CACHE STRING "Watcom PC98 libraries")

include(${CMAKE_CURRENT_LIST_DIR}/watcom_incl.cmake)
