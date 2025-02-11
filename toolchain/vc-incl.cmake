# Toolchain configuration for VC
# -----------------------------------------------
# Run cl to get the version and platform name.

find_program(CL_EXE cl.exe)
if(NOT CL_EXE)
    message(FATAL_ERROR "cl.exe not found. Please ensure that MSVC is installed and cl.exe is in the PATH.")
endif()

execute_process(
    COMMAND "${CL_EXE}" /?
    RESULT_VARIABLE CL_RESULT
    OUTPUT_VARIABLE CL_OUTPUT
    ERROR_VARIABLE CL_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
)
set(CL_FULL_OUTPUT "${CL_OUTPUT}\n${CL_ERROR}")
string(REGEX MATCH "Version ([0-9]+\\.[0-9]+\\.[0-9]+).*for[ ]+([A-Za-z0-9]+)" CL_VERSION_MATCH "${CL_FULL_OUTPUT}")
if(NOT CL_VERSION_MATCH)
    message(FATAL_ERROR "Failed to retrieve cl.exe version information. Output:\n${CL_FULL_OUTPUT}")
endif()
set(TOOLCHAIN_MSVC_VERSION "${CMAKE_MATCH_1}" CACHE STRING "MSVC Compiler Version" FORCE)
set(TOOLCHAIN_TARGET_ARCH "${CMAKE_MATCH_2}" CACHE STRING "Toolchain Target arch" FORCE)

#set(TOOLCHAIN_TARGET_PLATFORM "win64" CACHE STRING "Toolchain target platform" FORCE)
#if("${TOOLCHAIN_TARGET_ARCH}" STREQUAL "x86")
#  set(TOOLCHAIN_TARGET_PLATFORM "win32" CACHE STRING "Toolchain Target arch" FORCE)
#elseif("${TOOLCHAIN_TARGET_ARCH}" STREQUAL "ARM64")
#  set(TOOLCHAIN_TARGET_PLATFORM "winarm64" CACHE STRING "Toolchain Target arch" FORCE)
#endif()
#set(TOOLCHAIN_NAME "vc-${TOOLCHAIN_TARGET_PLATFORM}" CACHE STRING "Toolchain name" FORCE)
#----------------------------------------------------

# Use utf8 source.
if(TOOLCHAIN_MSVC_VERSION VERSION_GREATER_EQUAL "19.0.24215.1")
  add_compile_options(/utf-8)
endif()

# Use the c++ standard instead of the MS dialect.
add_compile_options(/Zc:wchar_t /Zc:forScope)

if(TOOLCHAIN_MSVC_VERSION VERSION_GREATER_EQUAL "19.0.24215.1")
  add_compile_options(/Zc:rvalueCast)
endif()

if(TOOLCHAIN_MSVC_VERSION VERSION_GREATER_EQUAL "19.14.26428.1")
  add_compile_options(/Zc:__cplusplus)
endif()

# win32 libraries.
set(TOOLCHAIN_ADD_LIBS "kernel32;user32;shell32;advapi32" CACHE STRING "Default Windows libraries")
