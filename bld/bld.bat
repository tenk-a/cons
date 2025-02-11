rem @echo off
:: Run
::  cmake -G "???" -DCMAKE_TOOLCHAIN_FILE=toolchain/???-toolchain.cmake -B bld/??? .
::  cmake --build bld/???
:: corresponding to the toolchain name in the argument.
:: [toolchain name]
::    vc-win64     vc-win32     vc-win64-md    vc-win32-md
::    mingw-win64  mingw-win32  djgpp
::    watcom-win32 watcom-dos32 watcom-dos16-s
::    borland-win32
pushd %~dp0
cd ..

set Toolchain=%1
set OPT0=%2
set GENE=
set COMPILER=
set ARCH=
set CRT=

if "%Toolchain%"=="" goto ERR_TOOLCHAIN_LIST

if not exist toolchain\%Toolchain%-toolchain.cmake goto ERR_TOOLCHAIN

for /f "tokens=1,2,3 delims=-" %%a in ("%Toolchain%") do (
    set "COMPILER=%%a"
    set "ARCH=%%b"
    set "CRT=%%c"
)

if /I not "%Toolchain:pc98=%"=="%Toolchain%" goto SKIP_PDCURSES
if /I not "%Toolchain:pcat=%"=="%Toolchain%" goto SKIP_PDCURSES
if exist thirdparty\lib\%Toolchain%\pdcurses.* goto SKIP_PDCURSES
call thirdparty\install_pdcurses.bat %COMPILER% %ARCH% %CRT%
:SKIP_PDCURSES

set OPT1=-DCMAKE_BUILD_TYPE=Release
set OPT2=

if /I "%COMPILER:~0,6%"=="watcom" set GENE=-G "Watcom WMake"
if /I "%COMPILER%"=="djgpp"   set GENE=-G "MinGW Makefiles"
if /I "%COMPILER%"=="mingw"   set GENE=-G "MinGW Makefiles"
if /I "%COMPILER%"=="borland" set GENE=-G "Borland Makefiles"

if /I not "%COMPILER:~0,2%"=="vc" goto L_CMAKE
set GENE=-G "NMake Makefiles"
set OPT1=
set OPT2=--config Release
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
@if /I not "%PATH:Microsoft Visual Studio 12.0=%"=="%PATH%" set GENE=-G "Visual Studio 12 2013%ARCH1%"
@if /I not "%PATH:Microsoft Visual Studio 14.0=%"=="%PATH%" set GENE=-G "Visual Studio 14 2015%ARCH1%"
@if /I not "%PATH:Microsoft Visual Studio\2017=%"=="%PATH%" set GENE=-G "Visual Studio 15 2017%ARCH1%"
@if /I not "%PATH:Microsoft Visual Studio\2019=%"=="%PATH%" set GENE=-G "Visual Studio 16 2019" %ARCH2%
@if /I not "%PATH:Microsoft Visual Studio\2022=%"=="%PATH%" set GENE=-G "Visual Studio 17 2022" %ARCH2%
:L_CMAKE
cmake %GENE% -DCMAKE_TOOLCHAIN_FILE=toolchain/%Toolchain%-toolchain.cmake %OPT0% %OPT1% -B bld/%Toolchain% .
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

:END
popd
