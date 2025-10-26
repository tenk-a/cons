#set(TOOLCHAIN_NAME "watcom-pcat-dos16-s" CACHE STRING "Toolchain name")

set(CMAKE_SYSTEM_NAME "dos")
set(CMAKE_SYSTEM_PROCESSOR "I86")
set(CMAKE_C_COMPILER "wcl")
set(CMAKE_CXX_COMPILER "wcl")

set(CMAKE_WATCOM_RUNTIME_LIBRARY "SingleThreaded")

set(CMAKE_C_FLAGS   "-ms -k4000 -D__DOS__ -D__DOSV__ ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-ms -k4000 -D__DOS__ -D__DOSV__ ${CMAKE_CXX_FLAGS}")

include(${CMAKE_CURRENT_LIST_DIR}/watcom_incl.cmake)
