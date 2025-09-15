#set(TOOLCHAIN_NAME "watcom-dos16-l" CACHE STRING "Toolchain name")

set(CMAKE_SYSTEM_NAME "dos")
set(CMAKE_SYSTEM_PROCESSOR "I86")
set(CMAKE_C_COMPILER "wcl")
set(CMAKE_CXX_COMPILER "wcl")

set(CMAKE_WATCOM_RUNTIME_LIBRARY "SingleThreaded")

set(CMAKE_C_FLAGS   "-ml -k6000 -D__DOS__ ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-ml -k6000 -D__DOS__ ${CMAKE_CXX_FLAGS}")

include(${CMAKE_CURRENT_LIST_DIR}/watcom_incl.cmake)
