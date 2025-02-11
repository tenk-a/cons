@echo off
::
:: This batch-file license: boost software license version 1.0
:: Please adjust to your compiler path if necessary.
::
goto L_START

:L_HELP
@echo USAGE: setcc [COMPILER] [win32/x64]
@echo   COMPILER:
@echo       vc143,vc142,vc141,vc140,vc120,vc110,vc100,vc90,vc80,vc71
@echo       msys2,msys2clang,mingw,watcom,dmc,orangec,djgpp
goto L_END

:L_START
set CcName=%1
set CcArch=%2

set INCLUDE=
set LIB=

if "%setcc_save_path%"=="" set "setcc_save_path=%path%"
set "setcc_base_path=%setcc_save_path%"

:: Host architecture
set CcHostArch=x64
if /I "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
    set "CcHostArch=x64"
) else if /I "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
    set "CcHostArch=arm64"
) else if /I "%PROCESSOR_ARCHITECTURE%"=="x86" (
    if defined PROCESSOR_ARCHITEW6432 (
        set "CcHostArch=x64"
    ) else (
        set "CcHostArch=x86"
    )
)

if /i "%ProgramFiles(x86)%"=="" set "ProgramFiles(x86)=%ProgramFiles%"

set COMPILER=
set VcVers_Args=

if "%CcArch%"==""         set CcArch=x64
if "%CcArch%"=="64"       set CcArch=x64
if /i "%CcArch%"=="amd64" set CcArch=x64
if "%CcArch%"=="32"       set CcArch=x86
if /i "%CcArch%"=="win32" set CcArch=x86
if /i "%CcArch%"=="arm64" set CcArch=arm64
if /i "%CcArch%"=="arm"   set CcArch=arm

if /i "%CcName%"=="vc143"      goto L_VC143
if /i "%CcName%"=="vc142"      goto L_VC142
if /i "%CcName%"=="vc141"      goto L_VC141
if /i "%CcName%"=="vc140"      goto L_VC14
if /i "%CcName%"=="vc120"      goto L_VC12
if /i "%CcName%"=="vc110"      goto L_VC11
if /i "%CcName%"=="vc100"      goto L_VC10
if /i "%CcName%"=="vc90"       goto L_VC9
if /i "%CcName%"=="vc80"       goto L_VC8
if /i "%CcName%"=="vc71"       goto L_VC71
if /i "%CcName%"=="vc70"       goto L_VC70
if /i "%CcName%"=="vc60"       goto L_VC6

if /i "%CcName%"=="msys"       goto L_MSYS2
if /i "%CcName%"=="msys2"      goto L_MSYS2
if /i "%CcName%"=="mingw"      goto L_MINGW
if /i "%CcName%"=="mingw32"    goto L_MINGW
if /i "%CcName%"=="gcc"        goto L_MSYS2
if /i "%CcName%"=="msys2clang" goto L_MSYS2_CLANG
if /i "%CcName%"=="clang"      goto L_MSYS2_CLANG

if /i "%CcName%"=="watcom"     goto L_WATCOM
if /i "%CcName%"=="ow"         goto L_WATCOM
if /i "%CcName%"=="ow19"       goto L_OW19
if /i "%CcName%"=="ow20"       goto L_OW20
if /i "%CcName%"=="dmc"        goto L_DMC
if /i "%CcName%"=="orangec"    goto L_ORANGEC
if /i "%CcName%"=="occ"        goto L_ORANGEC
if /i "%CcName%"=="borland"    goto L_BCC55
if /i "%CcName%"=="bc55"       goto L_BCC55
if /i "%CcName%"=="bcc101"     goto L_BCC101

if /i "%CcName%"=="djgpp"      goto L_DJGPP

@goto L_HELP


rem ## vc ######################################

:L_VC143
    set VcVer=vc143
    set VcYear=2022
    set "VsRoot=%ProgramFiles%\Microsoft Visual Studio\%VcYear%"
    goto L_VC14x_J2

:L_VC142
    set VcVer=vc142
    set VcYear=2019
    goto L_VC14x_J1

:L_VC141
    set VcVer=vc141
    set VcYear=2017

:L_VC14x_J1
    set "VsRoot=%ProgramFiles(x86)%\Microsoft Visual Studio\%VcYear%"

:L_VC14x_J2
    set "COMPILER=%VcVer%%CcArch%"
    set "VcVers_Args=%CcArch%"
    if /I not "%CcArch%"=="%CcHostArch%" set "VcVers_Args=%CcHostArch%_%CcArch%"
    if /I "%VcVers_Args%"=="x64_x86" set "VcVers_Args=x86"
    rem if /I "%VcVers_Args:~0,3%"=="x64" set "VcVers_Args=amd64%VcVers_Args:~3%"
    set "PATH=%setcc_base_path%"
    set "VsEdition="
    if exist "%VsRoot%\Community\Common7\Tools\VsMSBuildCmd.bat"    set "VsEdition=Community"
    if exist "%VsRoot%\Professional\Common7\Tools\VsMSBuildCmd.bat" set "VsEdition=Professional"
    if exist "%VsRoot%\Enterprise\Common7\Tools\VsMSBuildCmd.bat"   set "VsEdition=Enterprise"
    if "%VsEdition%"=="" goto L_VC14x_ERROR
    set "VsRoot=%VsRoot%\%VsEdition%"
    call "%VsRoot%\VC\Auxiliary\Build\vcvarsall.bat" %VcVers_Args%
    goto L_END
:L_VC14x_ERROR
    set VsRoot=
    echo ERROR: Not found "Microsoft Visual Studio %VcYear%"
    goto L_END

:L_VC14
    set VcVer=vc140
    set "COMPILER=%VcVer%%CcArch%"
    set "VS140WindowsSdk=%ProgramFiles(x86)%\Windows Kits\8.1"
    set "VcVers_Args=%CcArch%"
    if /I not "%CcArch%"=="%CcHostArch%" set "VcVers_Args=%CcHostArch%_%CcArch%"
    if /I "%VcVers_Args%"=="x64_x86" set "VcVers_Args=x86"
    pushd "%VS140COMNTOOLS%..\.."
    set "VcRoot=%CD%"
    popd
    set "PATH=%setcc_base_path%"
    if /I "%VcVers_Args%"=="x86"     call "%VS140COMNTOOLS%vsvars32.bat"
    if /I "%VcVers_Args%"=="x64"     call "%VcRoot%\vc\bin\amd64\vcvars64.bat"
    if /I "%VcVers_Args%"=="x86_x64" call "%VcRoot%\vc\bin\x86_amd64\vcvarsx86_amd64.bat"
    if /I "%VcVers_Args%"=="x64_arm" call "%VcRoot%\vc\bin\amd64_arm\vcvarsamd64_arm.bat"
    if /I "%VcVers_Args%"=="x86_arm" call "%VcRoot%\vc\bin\x86_arm\vcvarsx86_arm.bat"
    set "PATH=%VS140WindowsSdk%\bin\x86;%PATH%"
    goto L_END

:L_VC12
    set VcVer=vc120
    set "COMPILER=%VcVer%%CcArch%"
    set "PATH=%setcc_base_path%"
    if /I "%CcArch%"=="x64" goto L_VC12x64
    call "%VS120COMNTOOLS%vsvars32.bat"
    goto L_END
:L_VC12x64
    call "%VS120COMNTOOLS%..\..\vc\bin\amd64\vcvars64.bat"
    goto L_END

:L_VC11
    set VcVer=vc110
    set "COMPILER=%VcVer%%CcArch%"
    set "PATH=%setcc_base_path%"
    if /I "%CcArch%"=="x64" goto L_VC11x64
    call "%VS110COMNTOOLS%vsvars32.bat"
    goto L_END
:L_VC11x64
    call "%VS110COMNTOOLS%..\..\vc\bin\amd64\vcvars64.bat"
    goto L_END

:L_VC10
    set VcVer=vc100
    set "COMPILER=%VcVer%%CcArch%"
    set "PATH=%setcc_base_path%"
    if /I "%CcArch%"=="x64" goto L_VC10x64
    call "%VS100COMNTOOLS%vsvars32.bat"
    goto L_END
:L_VC10x64
    call "%VS100COMNTOOLS%..\..\vc\bin\amd64\vcvarsamd64.bat"
    goto L_END

:L_VC9
    set VcVer=vc90
    set "COMPILER=%VcVer%%CcArch%"
    set "PATH=%setcc_base_path%"
    if /I "%CcArch%"=="x64" goto L_VC9x64
    call "%VS90COMNTOOLS%vsvars32.bat"
    goto L_END
:L_VC9x64
    call "%VS90COMNTOOLS%..\..\vc\bin\amd64\vcvarsamd64.bat"
    goto L_END

:L_VC8
    set VcVer=vc80
    set "COMPILER=%VcVer%%CcArch%"
    set "PATH=%setcc_base_path%"
    if /I "%CcArch%"=="x64" goto L_VC8x64
    call "%VS80COMNTOOLS%vsvars32.bat"
    goto L_END
:L_VC8x64
    call "%VS80COMNTOOLS%..\..\vc\bin\amd64\vcvarsamd64.bat"
    goto L_END

:L_VC71
    set VcVer=vc71
    set "COMPILER=%VcVer%%CcArch%"
    set "PATH=%setcc_base_path%"
    if /i "%VS71_DIR%"=="" set "VS71_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio .NET 2003"
    if /i "%VS71COMNTOOLS%"=="" set "VS71COMNTOOLS=%VS71_DIR%\Common7\Tools\"
    set "INCLUDE=%VS71_DIR%\SDK\v1.1\include\"
    set "LIB=%VS71_DIR%\SDK\v1.1\Lib\"
    call "%VS71COMNTOOLS%vsvars32.bat"
    goto L_END

:L_VC70
    set VcVer=vc70
    set "COMPILER=%VcVer%%CcArch%"
    set "PATH=%setcc_base_path%"
    if /i "%VS70_DIR%"=="" set "VS70_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio .NET"
    if /i "%VS70COMNTOOLS%"=="" set "VS70COMNTOOLS=%VS70_DIR%\Common7\Tools\"
    call "%VS70COMNTOOLS%vsvars32.bat"
    goto L_END

:L_VC6
    set VcVer=vc60
    set "COMPILER=%VcVer%%CcArch%"
    set "PATH=%setcc_base_path%"
    if /i "%VS6_DIR%"=="" set "VS6_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio"
    if /i "%VS60COMNTOOLS%"=="" set "VS60COMNTOOLS=%VS6_DIR%\Common\Tools\"
    call "%VS6_DIR%\vc6\bin\vcvars32.bat"
    goto L_END

:: #############################

:L_MSYS2_CLANG
    if /i "%MSYS2_DIR%"=="" set "MSYS2_DIR=c:\msys64"
    if /I "%CcArch%"=="x64" goto L_MSYS2_CLANG64
    set COMPILER=msys2clang32
    set "PATH=%MSYS2_DIR%\clang32\bin;%MSYS2_DIR%\usr\bin;%setcc_base_path%"
    goto :L_END
:L_MSYS2_CLANG64
    set COMPILER=msys2clang64
    set "PATH=%MSYS2_DIR%\clang64\bin;%MSYS2_DIR%\usr\bin;%setcc_base_path%"
    goto :L_END

:L_MSYS2
    if /i "%MSYS2_DIR%"=="" set "MSYS2_DIR=c:\msys64"
    if /I "%CcArch%"=="x64" goto L_MSYS2_64
    set COMPILER=msys2mingw32
    set "PATH=%MSYS2_DIR%\mingw32\bin;%MSYS2_DIR%\usr\bin;%setcc_base_path%"
    goto :L_END
:L_MSYS2_64
    set COMPILER=msys2ucrt64
    set "PATH=%MSYS2_DIR%\ucrt64\bin;%MSYS2_DIR%\usr\bin;%setcc_base_path%"
    goto :L_END

:L_MINGW
    set COMPILER=mingw32
    if /i "%MINGW_DIR%"=="" set "MINGW_DIR=c:\MinGW"
    set "PATH=%MINGW_DIR%\bin;%MINGW_DIR%\msys\1.0\bin;%setcc_base_path%"
    goto :L_END

:L_CYGWIN
    if /I "%CcArch%"=="x64" goto L_CYGWIN64
    set COMPILER=cygwin32
    if /i "%CYGWIN32_DIR%"=="" set "CYGWIN_DIR32=c:\cygwin"
    set "PATH=%CYGWIN_DIR32%\bin;%setcc_base_path%"
    goto :L_END

:L_CYGWIN64
    set COMPILER=cygwin64
    if /i "%CYGWIN64_DIR%"=="" set "CYGWIN64_DIR=c:\cygwin64"
    set "PATH=%CYGWIN64_DIR%\bin;%setcc_base_path%"
    goto :L_END

:L_ORANGEC
    set COMPILER=orangec
    if /i "%ORANGEC_DIR%"=="" set "ORANGEC_DIR=C:\Program Files (x86)\Orange C 386"
    set "PATH=%ORANGEC_DIR%\bin;%setcc_base_path%"
    goto :L_END

:L_DMC
    set COMPILER=dmc
    if /i "%DMC_DIR%"=="" set "DMC_DIR=c:\dm"
    rem set "PATH=%DMC_DIR%\bin;%setcc_base_path%"
    set "PATH=d:\proj\cc_for_dmc\bin;%DMC_DIR%\bin;%setcc_base_path%"
    set "CC=dmc-cc.exe"
    set "CXX=dmc-cc.exe"
    goto :L_END

:L_OW19
    set COMPILER=ow19
    set "WATCOM=c:\watcom1.9"
    goto L_WATCOM_1
:L_OW20
    set COMPILER=ow20
    set "WATCOM=c:\watcom2.0"
    goto L_WATCOM_1
:L_WATCOM
    set COMPILER=watcom
:L_WATCOM_1
    if /i "%WATCOM%"=="" set "WATCOM=c:\watcom"
    set "PATH=%WATCOM%\BINNT;%WATCOM%\BINW;%setcc_base_path%"
    set "EDPATH=%WATCOM%\EDDAT"
    rem set "LIB=%PATCOM%\lib386;%PATCOM%\lib386\nt;%PATCOM%\lib386\nt\ddk;%PATCOM%\lib386\nt\directx"
    set "INCLUDE=%WATCOM%\H;%WATCOM%\H\NT;%WATCOM%\H\NT\DIRECTX;%WATCOM%\H\DDK;%INCLUDE%"
    set "FINCLUDE=%WATCOM%\SRC\FORTRAN"
    set "WHTMLHELP=%WATCOM%\BINNT\HELP"
    set "WIPFC=%WATCOM%\WIPFC"
    goto :L_END

:L_BCC55
    set COMPILER=bcc55
    if /i "%BCC55_DIR%"=="" set "BCC55_DIR=c:\borland\bcc55"
    set "PATH=%BCC55_DIR%\bin;%setcc_base_path%"
    set "INCLUDE=%BCC55_DIR%\include;%BCC55_DIR%\include\Rw;%BCC55_DIR%\include\psdk"
    goto :L_END

:L_BCC101
    set COMPILER=bcc101
    if /i "%BCC101_DIR%"=="" set "BCC101_DIR=c:\tools\bcc101"
    set "PATH=%BCC101_DIR%\bin;%setcc_base_path%"
    goto :L_END

:L_DJGPP
    set COMPILER=djgpp
    if /i "%MINGW_DIR%"=="" set "MINGW_DIR=c:\MinGW"
    if /i "%DJGPP_DIR%"=="" set "DJGPP_DIR=c:\djgpp"
    set "PATH=%DJGPP_DIR%\bin;%DJGPP_DIR%\i586-pc-msdosdjgpp\bin;%MINGW_DIR%\bin;%MINGW_DIR%\msys\1.0\bin;%setcc_base_path%"
    set  GCC_EXEC_PREFIX=%DJGPP_DIR%\lib\gcc\
    goto :L_END

:L_END
set VcVers_Args=
if /i "%CcArch%"=="x86" set "CcArch=win32"
set setcc_base_path=
rem pause
