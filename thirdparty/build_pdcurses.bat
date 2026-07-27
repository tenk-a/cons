rem @echo off
:: If necessary, git clone PDCurses|PDCursesMod
:: build pdcurses according to the compiler name argument,
:: and copy the files to include and lib/TARGET directory
:: under the thirdparty directory.
::  PDCurses PDcursesMod(default)
::  COMPILER ARCH     (Generate)
::  vc       win64  | vc-win64
::  vc       win32  | vc-win32
::  mingw    win64  | mingw-win64
::  mingw    win32  | mingw-win32
::  watcom          | watcom-win32 watcom-dos32 watcom-dos16-(s|c|m|l|h)
::  borland         | borland-win32
::  djgpp    dos32  | djgpp-dos32
setlocal
pushd %~dp0

set Compiler=
set Arch=
set BldTyp=
set Gen=
set Opt=
set CRT=
set LibPrefix=
set MAKE_CC=
set MAKE_CC_D=
set MAKE_CFLAGS=
set MAKE_CFLAGS_D=
set "MAKE_ARG=WIDE=Y UTF8=Y"
set "MAKE_ARG_D=%MAKE_ARG% DEBUG=Y"
::set "PDCurses=PDcurses"
set "PDCurses=PDCursesMod"

:ARG_LOOP
  if "%1"=="" goto ARG_LOOP_EXIT
  set arg=%1

  if /I "%arg%"=="PDCurses"     set "PDCurses=PDCurses"
  if /I "%arg%"=="PDCursesMod"  set "PDCurses=PDCursesMod"

  if /I "%arg:~0,2%"=="vc"      set "Compiler=%arg%"
  if /I "%arg:~0,6%"=="watcom"  set "Compiler=%arg%"
  if /I "%arg:~0,5%"=="mingw"   set "Compiler=%arg%"
  if /I "%arg:~0,5%"=="djgpp"   set "Compiler=%arg%"
  if /I "%arg:~0,7%"=="borland" set "Compiler=%arg%"

  if /I "%arg%"=="x86"          set Arch=win32
  if /I "%arg%"=="win32"        set Arch=win32
  if /I "%arg%"=="x64"          set Arch=win64
  if /I "%arg%"=="win64"        set Arch=win64
  if /I "%arg%"=="arm"          set Arch=arm
  if /I "%arg%"=="winarm"       set Arch=winarm
  if /I "%arg%"=="arm64"        set Arch=arm64
  if /I "%arg%"=="winarm64"     set Arch=winarm64

  if /I "%arg%"=="md"           set CRT=md
  ::if /I "%arg%"=="mt"         set CRT=

  shift
goto ARG_LOOP
:ARG_LOOP_EXIT

if exist %PDCurses% goto SKIP_GITHUB
if /I "%PDCurses%"=="PDCursesMod" (
  git clone https://github.com/Bill-Gray/PDCursesMod.git
) else (
  git clone https://github.com/wmcbrine/PDCurses.git
)
:SKIP_GITHUB
pushd %PDCurses%
git pull
popd

call :install_include

if "%Compiler%"=="" goto ERR_1
if /I "%Compiler:~0,2%"=="vc"      goto L_NMAKE
if /I "%Compiler:~0,6%"=="watcom"  goto L_WMAKE
if /I "%Compiler:~0,5%"=="mingw"   goto L_MingwMAKE
if /I "%Compiler:~0,5%"=="djgpp"   goto L_DjgppMAKE
if /I "%Compiler:~0,7%"=="borland" goto L_TMAKE
goto ERR_1

:L_NMAKE
if not "%Arch%"=="" goto L_NMAKE_ARCH_END
if "%Arch%"=="" call :ChkVcArch x64   win64
if "%Arch%"=="" call :ChkVcArch x86   win32
if "%Arch%"=="" call :ChkVcArch ARM64 winarm64
if "%Arch%"=="" call :ChkVcArch ARM   winarm
:L_NMAKE_ARCH_END
if /I "%Arch%"=="arm64" set Arch=winarm64
if /I "%Arch%"=="arm"   set Arch=winarm
set make=nmake
set Makefile=Makefile.vc
set ext=lib
set "TMP_CFLAGS=-WX- -W4 -D_CRT_SECURE_NO_WARNINGS"
set MAKE_CFLAGS="CFLAGS=-Ox %TMP_CFLAGS%"
set MAKE_CFLAGS_D="CFLAGS=-Z7 -DPDCDEBUG %TMP_CFLAGS%"
if /I "%CRT%"=="md" goto L_NMAKE_VC_MD
set MAKE_CC="CC=cl -nologo -MT"
set MAKE_CC_D="CC=cl -nologo -MTd"
goto L_NMAKE_SKIP_VC
:L_NMAKE_VC_MD
set MAKE_CC="CC=cl -nologo -WX- -MD"
set MAKE_CC_D="CC=cl -nologo -WX- -MDd"
:L_NMAKE_SKIP_VC
call :win_compile
goto END

:ChkVcArch
cl.exe 2>&1 | findstr /C:"%1" >nul
if %ERRORLEVEL% equ 0 set Arch=%2
:L_ChkVcArch_End
exit /b 0

:L_MingwMAKE
set make=make
set Makefile=Makefile
set ext=a
set LibPrefix=lib
set MAKE_CC="CC=gcc -DNDEBUG" "ON_WINDOWS=1"
set MAKE_CC_D="PREFIX=" "ON_WINDOWS=1"
call :win_compile
goto END

:L_DjgppMAKE
set make=make
set Makefile=Makefile
set ext=a
set LibPrefix=lib
set LibDir=%Compiler%-dos32
set "WorkDir=dos"
set "MAKE_ARG="
set "MAKE_ARG_D=DEBUG_=Y"
call :compile
goto END

:L_WMAKE
set make=wmake
set Makefile=Makefile.wcc
set ext=lib
set Arch=win32
call :win_compile
call :dos32compile
call :dos16compile s
call :dos16compile c
call :dos16compile m
call :dos16compile l
call :dos16compile h
goto END

:L_TMAKE
set Arch=win32
set make=tmake
set Makefile=Makefile.bcc
set ext=lib
call :win_compile
goto END

:install_include
set incdir=%CD%\include
if not exist %incdir% mkdir %incdir%
copy /b %PDCurses%\*.h %incdir%\
exit /b 0

:dos32compile
set LibDir=%Compiler%-dos32
set MDL=f
goto L_DOS_COMPILE

:dos16compile
set MDL=%1
set LibDir=%Compiler%-dos16-%MDL%

:L_DOS_COMPILE
set "WorkDir=dos"
set "MAKE_ARG=MODEL=%MDL%"
if /I not "%MDL%"=="f" set "MAKE_ARG=%MAKE_ARG% CHTYPE_32=Y"
set "MAKE_ARG_D=%MAKE_ARG% DEBUG_=Y"
goto compile

:win_compile
set WorkDir=wincon
set LibDir=%Compiler%
if not "%Arch%"=="" set LibDir=%Compiler%-%Arch%
if not "%CRT%"==""  set LibDir=%Compiler%-%Arch%-%CRT%

:compile
set dstdir=%CD%\lib\%LibDir%
if not exist %dstdir% mkdir %dstdir%
set dstdirD=%CD%\lib\debug\%LibDir%
if not exist %dstdirD% mkdir %dstdirD%

pushd %PDCurses%\%WorkDir%
del *.obj *.o *.lib *.a *.pdb *.map *.ilb *.bak *.err >nul

%make% -f %Makefile% %MAKE_ARG% %MAKE_CC% %MAKE_CFLAGS%
copy /b pdcurses.%ext% %dstdir%\%LibPrefix%pdcurses.%ext%
del *.obj *.o *.lib *.a *.pdb *.map *.ilb *.bak *.err >nul

%make% -f %Makefile% %MAKE_ARG_D% %MAKE_CC_D% %MAKE_CFLAGS_D%
copy /b pdcurses.%ext% %dstdirD%\%LibPrefix%pdcurses.%ext%
del *.obj *.o *.lib *.a *.pdb *.map *.ilb *.bak *.err >nul
popd
exit /b 0

:ERR_1
@echo install_pdcurses [Compiler]
@echo   Compiler vc mingw watcom borland djgpp
goto END

:END

popd
endlocal
