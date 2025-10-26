# vc toolchain
find_program(CL_EXE cl.exe)
if(NOT CL_EXE)
  message(FATAL_ERROR "cl.exe not found. Please ensure that MSVC is installed and cl.exe is in the PATH.")
endif()

execute_process(
  COMMAND "${CL_EXE}" /?
  RESULT_VARIABLE CL_RESULT
  OUTPUT_VARIABLE CL_OUTPUT
  ERROR_VARIABLE  CL_ERROR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE
)
set(CL_FULL_OUTPUT "${CL_OUTPUT}\n${CL_ERROR}")
string(REGEX MATCH "Version ([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+).*for[ ]+([A-Za-z0-9]+)" CL_VERSION_MATCH "${CL_FULL_OUTPUT}")
if(NOT CL_VERSION_MATCH)
    string(REGEX MATCH "Version ([0-9]+\\.[0-9]+\\.[0-9]).*for[ ]+([A-Za-z0-9]+)" CL_VERSION_MATCH "${CL_FULL_OUTPUT}")
  if(NOT CL_VERSION_MATCH)
    message(FATAL_ERROR "Failed to retrieve cl.exe version information. Output:\n${CL_FULL_OUTPUT}")
  endif()
endif()
set(TOOLCHAIN_MSVC_VERSION "${CMAKE_MATCH_1}" CACHE STRING "MSVC Compiler Version" FORCE)
set(TOOLCHAIN_TARGET_ARCH  "${CMAKE_MATCH_2}" CACHE STRING "Toolchain Target arch" FORCE)

#set(TOOLCHAIN_TARGET_PLATFORM "win64" CACHE STRING "Toolchain target platform" FORCE)
#if("${TOOLCHAIN_TARGET_ARCH}" STREQUAL "x86")
#  set(TOOLCHAIN_TARGET_PLATFORM "win32" CACHE STRING "Toolchain Target arch" FORCE)
#elseif("${TOOLCHAIN_TARGET_ARCH}" STREQUAL "ARM64")
#  set(TOOLCHAIN_TARGET_PLATFORM "winarm64" CACHE STRING "Toolchain Target arch" FORCE)
#endif()
#set(TOOLCHAIN_NAME "vc-${TOOLCHAIN_TARGET_PLATFORM}" CACHE STRING "Toolchain name" FORCE)

#set(CMAKE_MAKE_PROGRAM "nmake")

#----------------------------------------------------
# app setting

set(TOOLCHAIN_ADD_LIBS       "kernel32;user32;shell32;advapi32" CACHE STRING "Default Windows libraries")
#set(TOOLCHAIN_ADD_LINK_OPTS "/SUBSYSTEM:CONSOLE" CACHE STRING "TOOLCHAIN_ADD_LINK_OPTS")

set(ADD_OPTS "")

if(TOOLCHAIN_MSVC_VERSION VERSION_GREATER_EQUAL "19.00.24215.1") # >=vc2015upd3
  # UTF-8 (ANSI) API
  set(ADD_OPTS "${ADD_OPTS} /utf-8 /U_MBCS /UUNICODE /U_UNICODE")
  set(ADD_OPTS "${ADD_OPTS} /DCONS_USE_UNICODE")
  # Use Windows UTF-8 API (windows10 1903 or later)
  set(TOOLCHAIN_ADD_SRCS "${CMAKE_CURRENT_LIST_DIR}/../src/win/ActiveCodePageUTF8.manifest" CACHE STRING "TOOLCHAIN_ADD_SRCS")
else()
  # UNICODE API
  set(ADD_OPTS "${ADD_OPTS} /U_MBCS /DUNICODE /D_UNICODE")
  set(ADD_OPTS "${ADD_OPTS} /UCONS_USE_UNICODE")
endif()

#----------------------------------------------------
# c/c++ options

set(ADD_OPTS          "${ADD_OPTS} /D_CRT_SECURE_NO_WARNINGS=1 /D_CRT_NONSTDC_NO_DEPRECATE=1")
#set(ADD_OPTS         "${ADD_OPTS} /D_Pragma=__pragma")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

set(CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS} /Zc:wchar_t /Zc:forScope")
if(TOOLCHAIN_MSVC_VERSION VERSION_GREATER_EQUAL "19.00.24215.1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:rvalueCast")
endif()
if(TOOLCHAIN_MSVC_VERSION VERSION_GREATER_EQUAL "19.14.26428.1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
endif()

if(TOOLCHAIN_MSVC_VERSION VERSION_LESS "19.0") # vc2013 and earlier
  #message(STATUS "vc2013 and earlier")

  # set(TOOLCHAIN_USE_CCWRAP 1)
  if(TOOLCHAIN_USE_CCWRAP)
    set(CCWRAP_DIR "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/ccwrap")
    if(EXISTS "${CCWRAP_DIR}" AND IS_DIRECTORY "${CCWRAP_DIR}")
      set(ADD_OPTS "-I${CCWRAP_DIR}/vc/ -I${CCWRAP_DIR}/ccwrap/ -FI${CCWRAP_DIR}/vc/ccwrap_header.h ${ADD_OPTS}")
    else()
      message(WARNING "thirdparty/ccwrap directory is missing. For vc12 and earlier, run thirdparty/install_ccwap.bat")
      unset(TOOLCHAIN_USE_CCWRAP)
    endif()
  endif()
  if(NOT TOOLCHAIN_USE_CCWRAP)
    set(ADD_OPTS        "${ADD_OPTS}        /D_XKEYCHECK_H")
    set(ADD_OPTS        "${ADD_OPTS}        /Dsnprintf=_snprintf")
    set(ADD_OPTS        "${ADD_OPTS}        /Dstrdup=_strdup")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /Dinline=__inline")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Dnoexcept=throw()")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Dconstexpr=const")
    if(TOOLCHAIN_MSVC_VERSION VERSION_LESS "18.0") # vc2012 and earlier
      set(ADD_OPTS      "${ADD_OPTS} /Dstrtoll=_strtoi64 /Dstrtoull=_strtoui64 /Datoll=_atoi64")
      set(ADD_OPTS      "${ADD_OPTS} /Dwcstoll=_wcstoi64 /Dwcstoull=_wcstoui6")
    endif()
    if(TOOLCHAIN_MSVC_VERSION VERSION_LESS "17.0") # vc2010 and earlier
      set(ADD_OPTS "-I${CMAKE_CURRENT_LIST_DIR}/../src/misc/workround/vc/ ${ADD_OPTS}")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Doverride=")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Dfinal=sealed")
    endif()
    if(TOOLCHAIN_MSVC_VERSION VERSION_LESS "16.0") # vc2008 and earlier
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Dnullptr=0")
     #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Dchar16_t=unsigned\\ short /Dchar32_t=unsigned\\ int")
    endif()
  endif()
endif()

set(CMAKE_C_FLAGS   "${ADD_OPTS} ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${ADD_OPTS} ${CMAKE_CXX_FLAGS}")
