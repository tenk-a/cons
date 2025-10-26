@echo off
:: usage> bld [toolchain-name] [debug]
::  Run
::   cmake -G "???" -DCMAKE_TOOLCHAIN_FILE=toolchain/???-toolchain.cmake -B bld/??? .
::   cmake --build bld/???
::  corresponding to the toolchain name in the argument.
::  [toolchain name]
::    vc-win64     vc-win32     vc-win64-md    vc-win32-md
::    mingw-win64  mingw-win32  djgpp
::    watcom-win32 watcom-dos32 watcom-dos16-s
::    borland-win32
::
:: usage> bld list
::  list toolchains.
pushd %~dp0
cd ..

set Toolchain=%1
set OPT0=%2
set GENE=
set GENE2=
set COMPILER=
set ARCH=
set CRT=

if /I "%Toolchain%"=="list"    goto ERR_TOOLCHAIN_LIST

set CMAKE_VER=00000
call :GET_CMAKE_VER

if /I "%Toolchain%"==""        call :GET_TOOLCHAIN

for /f "tokens=1,2,3 delims=-" %%a in ("%Toolchain%") do (
    set "COMPILER=%%a"
    set "ARCH=%%b"
    set "CRT=%%c"
)

if /I "%COMPILER:~0,2%"=="vc" call :GET_VC_VER

if not exist toolchain\%Toolchain%-toolchain.cmake goto ERR_TOOLCHAIN

:: PDCurses check
if /I not "%Toolchain:pc98=%"=="%Toolchain%" goto SKIP_PDCURSES
if /I not "%Toolchain:pcat=%"=="%Toolchain%" goto SKIP_PDCURSES
if /I not "%Toolchain:dosv=%"=="%Toolchain%" goto SKIP_PDCURSES
if exist thirdparty\lib\%Toolchain%\*pdcurses.* goto SKIP_PDCURSES
call thirdparty\install_pdcurses.bat %COMPILER% %ARCH% %CRT%
:SKIP_PDCURSES

:: Options
set OPT1=-DCMAKE_BUILD_TYPE=Release
set OPT2=
set debug_bld=
if /I not "%OPT0%"=="debug" goto L_SKIP_DEBUG
set "OPT0="
set "OPT1=-DCMAKE_BUILD_TYPE=Debug"
set "debug_bld=1"
:L_SKIP_DEBUG

:: Select Generater
if /I "%COMPILER:~0,6%"=="watcom" set "GENE=Watcom WMake"
if /I "%COMPILER%"=="djgpp"       set "GENE=MinGW Makefiles"
if /I "%COMPILER%"=="mingw"       set "GENE=MinGW Makefiles"
if /I "%COMPILER%"=="borland"     set "GENE=Borland Makefiles"

if /I not "%COMPILER:~0,2%"=="vc" goto L_CMAKE

:: Visual C/C++
::set VC_VER=%COMPILER:~2%
::call :GET_VC_VER
set ARCH1=
set "ARCH2=-A Win32"
if /I "%ARCH%"=="win32"    goto SKIP_ARCH_E
if /I "%ARCH%"=="winarm64" goto L_VC_ARM64
if /I "%ARCH%"=="winarm"   goto L_VC_ARM
set "ARCH1= Win64"
set "ARCH2=-A x64"
goto SKIP_ARCH_E
:L_VC_ARM64
:: for vc142 vc143
set "ARCH1= ARM"
set "ARCH2=-A arm64"
goto SKIP_ARCH_E
:L_VC_ARM
:: for vc142 vc143
set "ARCH1= ARM"
set "ARCH2=-A arm"
goto SKIP_ARCH_E

:SKIP_ARCH_E
set "GENE=NMake Makefiles"
@if %CMAKE_VER% lss 40000 goto L_CMAKE3
@if "%VC_VER%"=="140" set "GENE=Visual Studio 14 2015"
@if "%VC_VER%"=="141" set "GENE=Visual Studio 15 2017"
@if "%VC_VER%"=="142" set "GENE=Visual Studio 16 2019"
@if "%VC_VER%"=="143" set "GENE=Visual Studio 17 2022"
@if /I "%GENE%"=="NMake Makefiles" goto L_CMAKE
set "GENE2=%ARCH2%"
goto L_CMAKE_SKIP2
:L_CMAKE3
@if %CMAKE_VER% lss 30000 goto L_CMAKE
@if %CMAKE_VER% lss 31200 if "%VC_VER%"=="80"  set "GENE=Visual Studio 8 2005%ARCH1%"
@if %CMAKE_VER% lss 33000 if "%VC_VER%"=="90"  set "GENE=Visual Studio 9 2008%ARCH1%"
@if %CMAKE_VER% lss 32500 if "%VC_VER%"=="100" set "GENE=Visual Studio 10 2010%ARCH1%"
@if %CMAKE_VER% lss 32800 if "%VC_VER%"=="110" set "GENE=Visual Studio 11 2012%ARCH1%"
@if %CMAKE_VER% lss 33100 if "%VC_VER%"=="120" set "GENE=Visual Studio 12 2013%ARCH1%"
@if %CMAKE_VER% lss 40000 if "%VC_VER%"=="140" set "GENE=Visual Studio 14 2015%ARCH1%"
@if %CMAKE_VER% lss 40000 if "%VC_VER%"=="141" set "GENE=Visual Studio 15 2017%ARCH1%"
@if "%VC_VER%"=="142" set "GENE=Visual Studio 16 2019"
@if "%VC_VER%"=="143" set "GENE=Visual Studio 17 2022"
@if %VC_VER% geq 142  set "GENE2=%ARCH2%"
@if /I "%GENE%"=="NMake Makefiles" goto L_CMAKE
:L_CMAKE_SKIP2
set OPT1=
set OPT2=--config Release
if "%debug_bld%"=="1" set OPT2=--config Debug

:: CMake
:L_CMAKE
cmake -G "%GENE%" %GENE2% -DCMAKE_TOOLCHAIN_FILE=toolchain/%Toolchain%-toolchain.cmake %OPT0% %OPT1% -B bld/%Toolchain% .
cmake --build bld/%Toolchain% %OPT2%
cmake --install bld/%Toolchain%

goto END

:ERR_TOOLCHAIN
@echo ERROR: No toolchain : %Toolchain%
@echo:
:ERR_TOOLCHAIN_LIST
@echo Usage: bld [TOOLCHAIN]
@for %%a in (toolchain\*-toolchain.cmake) do @call :PUT_TOOLCHAIN_NAME %%a
goto END

:PUT_TOOLCHAIN_NAME
@set NAME=%1
@set NAME=%NAME:-toolchain.cmake=%
@set NAME=%NAME:toolchain\=%
@echo 	%NAME%
@exit /b 0

:: ===========================================================================
:GET_CMAKE_VER
set "CMAKE_VER="
set "CMAKE_FIRSTLINE="

for /f "usebackq delims=" %%L in (`cmake --version 2^>nul`) do (
    set "CMAKE_FIRSTLINE=%%L"
    goto :L_CMAKE_VER_PARSE
)
rem not found.
set "CMAKE_VER=00000"
exit /b 0

:L_CMAKE_VER_PARSE
for %%T in (%CMAKE_FIRSTLINE%) do set "CMAKE_VER_TOKEN=%%T"

for /f "tokens=1 delims=-" %%V in ("%CMAKE_VER_TOKEN%") do set "CMAKE_VER=%%V"

set "MAJ="
set "MIN="
set "PAT="
for /f "tokens=1-3 delims=." %%A in ("%CMAKE_VER%") do (
    set "MAJ=%%A"
    set "MIN=%%B"
    set "PAT=%%C"
)

if not defined MAJ set "MAJ=0"
if not defined MIN set "MIN=0"
if not defined PAT set "PAT=0"

set "MIN=0%MIN%"
set "MIN=%MIN:~-2%"
set "PAT=0%PAT%"
set "PAT=%PAT:~-2%"

set /a "CMAKE_VER=%MAJ%%MIN%%PAT%"
::@echo CMAKE_VER=%CMAKE_VER%

set "MAJ="
set "MIN="
set "PAT="
set "CMAKE_VER_TOKEN="
set "CMAKE_FIRSTLINE="

exit /b 0

:: ===========================================================================
:GET_TOOLCHAIN
set "Toolchain="
@if /I not "%PATH:borland=%"=="%PATH%" set Toolchain=borland-win32
::@if /I not "%PATH:dm\bin=%"=="%PATH%"  set Toolchain=dmc
::@if /I not "%PATH:dmc\bin=%"=="%PATH%" set Toolchain=dmc
@if /I not "%PATH:mingw=%"=="%PATH%"   set Toolchain=mingw
@if /I not "%PATH:msys32=%"=="%PATH%"  set Toolchain=mingw-win32
@if /I not "%PATH:msys64=%"=="%PATH%"  set Toolchain=mingw-win64
@if /I not "%PATH:msys64\clang32=%"=="%PATH%" set Toolchain=mingw-win32
@if /I not "%PATH:msys64\mingw32=%"=="%PATH%" set Toolchain=mingw-win32
@if /I not "%PATH:djgpp=%"=="%PATH%"   set Toolchain=djgpp-dos32
@if /I not "%PATH:WATCOM=%"=="%PATH%"  set Toolchain=watcom-win32
@if not "%Toolchain%"=="" goto L_TOOLCHAIN_END
set "VC_VER="
@if /I not "%PATH:Microsoft Visual Studio .NET 2003=%"=="%PATH%" set VC_VER=71
@if /I not "%PATH:Microsoft Visual Studio 8=%"=="%PATH%"    set VC_VER=80
@if /I not "%PATH:Microsoft Visual Studio 9.0=%"=="%PATH%"  set VC_VER=90
@if /I not "%PATH:Microsoft Visual Studio 10.0=%"=="%PATH%" set VC_VER=100
@if /I not "%PATH:Microsoft Visual Studio 11.0=%"=="%PATH%" set VC_VER=110
@if /I not "%PATH:Microsoft Visual Studio 12.0=%"=="%PATH%" set VC_VER=120
@if /I not "%PATH:Microsoft Visual Studio 14.0=%"=="%PATH%" set VC_VER=140
@if /I not "%PATH:Microsoft Visual Studio\2017=%"=="%PATH%" set VC_VER=141
@if /I not "%PATH:Microsoft Visual Studio\2019=%"=="%PATH%" set VC_VER=142
@if /I not "%PATH:Microsoft Visual Studio\2022=%"=="%PATH%" set VC_VER=143
@if "%VC_VER%"=="" goto L_TOOLCHAIN_END
::L_VC
::if /I "%VC_VER:~0,2%"=="vc" set /a "VC_VER=%VC_VER:~2%"
set /a VC_VER=%VC_VER%
set VC_ARCH=
::if /I not "%VC_ARCH%"=="" goto L_SKIP_ARCH
@if "%VC_VER%"=="143"     if /I not "%PATH:\bin\HostX64\x64=%"=="%PATH%" set VC_ARCH=win64
@if "%VC_VER%"=="142"     if /I not "%PATH:\bin\HostX64\x64=%"=="%PATH%" set VC_ARCH=win64
@if "%VC_VER%"=="141"     if /I not "%PATH:\bin\HostX64\x64=%"=="%PATH%" set VC_ARCH=win64
@if /I not "%PATH:Microsoft Visual Studio 14.0\VC\BIN\amd64=%"=="%PATH%" set VC_ARCH=win64
@if /I not "%PATH:Microsoft Visual Studio 13.0\VC\BIN\amd64=%"=="%PATH%" set VC_ARCH=win64
@if /I not "%PATH:Microsoft Visual Studio 12.0\VC\BIN\amd64=%"=="%PATH%" set VC_ARCH=win64
@if /I not "%PATH:Microsoft Visual Studio 11.0\VC\BIN\amd64=%"=="%PATH%" set VC_ARCH=win64
@if /I not "%PATH:Microsoft Visual Studio 10.0\VC\BIN\amd64=%"=="%PATH%" set VC_ARCH=win64
@if /I not "%PATH:Microsoft Visual Studio 9.0\VC\BIN\amd64=%"=="%PATH%"  set VC_ARCH=win64
@if /I not "%PATH:Microsoft Visual Studio 8\VC\BIN\amd64=%"=="%PATH%"    set VC_ARCH=win64
:L_SKIP_ARCH
if /I "%VC_ARCH%"=="" set VC_ARCH=win32
set Toolchain=vc-%VC_ARCH%
set VC_ARCH=
:L_TOOLCHAIN_END
exit /b 0

:: ===========================================================================

:GET_VC_VER
setlocal EnableExtensions EnableDelayedExpansion
set MSC_VER=
set VC_ARCH=

where cl >nul 2>nul || goto END_GET_VC_VER

set "banner="
for /f "usebackq delims=" %%A in (`cl 2^>^&1 ^| findstr /i "Version"`) do if not defined banner set "banner=%%A"
if not defined banner goto END_GET_VC_VER

set "afterV="
for /f "tokens=1,* delims=V" %%a in ("!banner!") do set "afterV=%%b"
if not defined afterV goto END_GET_VC_VER

set "afterV=!afterV:~6!"
for /f "tokens=1 delims= " %%v in ("!afterV!") do set "verTok=%%v"
if not defined verTok goto END_GET_VC_VER

for /f "tokens=1,2 delims=." %%M in ("!verTok!") do (
  set "maj=%%M"
  set "min=%%N"
)
if not defined maj goto END_GET_VC_VER

set "min2=!min!0"
set "min2=!min2:~0,2!"
set "MSC_VER=!maj!!min2!"

echo(!banner! | findstr /i "ARM64" >nul && set "VC_ARCH=winarm64"
if /i "!VC_ARCH!"=="" (
  echo(!banner! | findstr /i "ARM "  >nul && set "VC_ARCH=winarm"
)
if /i "!VC_ARCH!"=="" (
  echo(!banner! | findstr /i " x64"   >nul && set "VC_ARCH=win64"
)
if /i "!VC_ARCH!"=="" (
  echo(!banner! | findstr /i "Win64" >nul && set "VC_ARCH=win64"
)
if /i "!VC_ARCH!"=="" (
  echo(!banner! | findstr /i " x86"  >nul && set "VC_ARCH=win32"
)
if /i "!VC_ARCH!"=="" (
  echo(!banner! | findstr /i "Win32" >nul && set "VC_ARCH=win32"
)
if /i "!VC_ARCH!"=="" set "VC_ARCH=win32"

:END_GET_VC_VER
endlocal & (
  set "MSC_VER=%MSC_VER%"
  set "VC_ARCH=%VC_ARCH%"
)
@if %MSC_VER% geq 1400 set VC_VER=80
@if %MSC_VER% geq 1500 set VC_VER=90
@if %MSC_VER% geq 1600 set VC_VER=100
@if %MSC_VER% geq 1700 set VC_VER=110
@if %MSC_VER% geq 1800 set VC_VER=120
@if %MSC_VER% geq 1900 set VC_VER=140
@if %MSC_VER% geq 1910 set VC_VER=141
@if %MSC_VER% geq 1920 set VC_VER=142
@if %MSC_VER% geq 1930 set VC_VER=143
@if %MSC_VER% geq 1930 set VC_VER=143
if /I "%ARCH%"=="%VC_ARCH%" goto L_SKIP_VC_VER_2
set ARCH=%VC_ARCH%
set Toolchain=%COMPILER%-%ARCH%
if /I not "%CRT%"=="" set Toolchain=%COMPILER%-%ARCH%-%CRT%
:L_SKIP_VC_VER_2
@echo VC_VER=%VC_VER%
@echo MSC_VER=%MSC_VER%
@echo VC_ARCH=%VC_ARCH%
exit /b 0

:: ===========================================================================

:END
popd
