@echo off
::
:: コンパイラを設定してhelloのビルドを行う確認用バッチ.
:: 引数無しで全ビルドを想定.
:: ビルド環境のコンパイラ次第なので、環境毎に書換る.
::
@if not "%1"=="" (
  @echo:
  @echo:
  @echo [[%1]]
  @echo:
)
@if /I "%1"=="VC_WIN64" goto BLD_VC_WIN64
@if /I "%1"=="VC_WIN32" goto BLD_VC_WIN32
@if /I "%1"=="VC_ARM64" goto BLD_VC_ARM64
@if /I "%1"=="VC_ARM32" goto BLD_VC_ARM32
@if /I "%1"=="MSYS64"   goto BLD_MSYS64
@if /I "%1"=="MSYS32"   goto BLD_MSYS32
@if /I "%1"=="WATCOM"   goto BLD_WATCOM
@if /I "%1"=="DJGPP"    goto BLD_DJGPP
@if /I "%1"=="BORLAND"  goto BLD_BORLAND
@if /I "%1"=="VC90_WIN32" goto BLD_VC90_WIN32
@if not "%1"=="" goto ERR

:: all build.
chcp 932
cmd /c all_bld.bat BORLAND
cmd /c all_bld.bat VC90_WIN32
chcp 65001
cmd /c all_bld.bat WATCOM
cmd /c all_bld.bat MSYS64
cmd /c all_bld.bat MSYS32
cmd /c all_bld.bat DJGPP
cmd /c all_bld.bat VC_WIN64
cmd /c all_bld.bat VC_WIN32
cmd /c all_bld.bat VC_ARM64
cmd /c all_bld.bat VC_ARM32
goto END


:BLD_VC_WIN64
call setcc.bat vc143 x64
call bld.bat vc-win64
call bld.bat vc-win64-md
goto END

:BLD_VC_WIN32
call setcc.bat vc143 win32
call bld.bat vc-win32
call bld.bat vc-win32-md
goto END

:BLD_VC_ARM64
call setcc.bat vc143 arm64
call bld.bat vc-winarm64
goto END

:BLD_VC_ARM32
call setcc.bat vc143 arm
call bld.bat vc-winarm
goto END

:BLD_VC90_WIN32
copy ..\toolchain\vc-win32-toolchain.cmake    ..\toolchain\vc90-win32-toolchain.cmake
copy ..\toolchain\vc-win32-md-toolchain.cmake ..\toolchain\vc90-win32-md-toolchain.cmake
call setcc.bat vc90 win32
call bld.bat vc90-win32
call bld.bat vc90-win32-md
del ..\toolchain\vc90-*.cmake
goto END

:BLD_WATCOM
call setcc.bat watcom
call bld.bat watcom-win32
call bld.bat watcom-dos32
call bld.bat watcom-dos16-s
goto END

:BLD_MSYS64
call setcc.bat msys2 x64
call bld.bat mingw-win64
goto END

:BLD_MSYS32
call setcc.bat msys2 win32
call bld.bat mingw-win32
goto END

:BLD_DJGPP
call setcc.bat djgpp
call bld.bat djgpp-dos32
goto END

:BLD_BORLAND
call setcc.bat bcc101
call bld.bat borland-win32
goto END

:ERR
@echo Invalid argument : %1
goto END

:END
