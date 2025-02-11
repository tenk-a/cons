#set(TOOLCHAIN_NAME "mac" CACHE STRING "Toolchain name")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(TOOLCHAIN_ADD_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/thirdparty/include:/opt/homebrew/include" CACHE STRING "Default include directories for the toolchain")
    set(TOOLCHAIN_ADD_LINK_DIRS "${CMAKE_SOURCE_DIR}/thirdparty/lib/armmac:/opt/homebrew/lib" CACHE STRING "Default library directories for the toolchain")
else()
    set(TOOLCHAIN_ADD_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/thirdparty/include:/usr/local/include" CACHE STRING "Default include directories for the toolchain")
    set(TOOLCHAIN_ADD_LINK_DIRS "${CMAKE_SOURCE_DIR}/thirdparty/lib/armmac:/usr/local/lib" CACHE STRING "Default library directories for the toolchain")
endif()
