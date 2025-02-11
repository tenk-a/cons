#set(TOOLCHAIN_NAME "vc-win32" CACHE STRING "Toolchain name" FORCE)

include(${CMAKE_CURRENT_LIST_DIR}/vc-incl.cmake)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
