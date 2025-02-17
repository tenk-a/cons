@echo off
::
:: コンパイラを設定してhelloのビルドを行う確認用バッチ.
:: 引数無しで全ビルドを想定.
:: ビルド環境のコンパイラ次第なので、環境毎に書換る.
::
@if not "%1"=="" (
  @echo:
  @echo:
  @echo [[%1]] %2
  @echo:
)
@if /I "%1"=="VC-WIN64" goto BLD_VC_WIN64
@if /I "%1"=="VC-WIN32" goto BLD_VC_WIN32
@if /I "%1"=="VC-ARM64" goto BLD_VC_ARM64
@if /I "%1"=="VC-ARM32" goto BLD_VC_ARM32
@if /I "%1"=="MSYS64"   goto BLD_MSYS64
@if /I "%1"=="MSYS32"   goto BLD_MSYS32
@if /I "%1"=="WATCOM"   goto BLD_WATCOM
@if /I "%1"=="DJGPP"    goto BLD_DJGPP
@if /I "%1"=="BORLAND"  goto BLD_BORLAND
@if /I "%1"=="VCVER-WIN32"  goto BLD_VCVER_WIN32
@if /I "%1"=="ia16-elf-exe" goto BLD_IA16_ELF_EXE
@if /I "%1"=="linux"    goto LINUX
@if not "%1"=="" goto ERR

:: all build.
chcp 65001
cmd /c all_bld.bat WATCOM
cmd /c all_bld.bat MSYS64
cmd /c all_bld.bat MSYS32
cmd /c all_bld.bat DJGPP
cmd /c all_bld.bat VC-WIN64
cmd /c all_bld.bat VC-WIN32
cmd /c all_bld.bat VC-ARM64
cmd /c all_bld.bat VC-ARM32
rem goto END

cmd /c all_bld.bat ia16-elf-exe
cmd /c all_bld.bat linux
cmd /c all_bld.bat VCVER-WIN32 140
cmd /c all_bld.bat VCVER-WIN32 141
cmd /c all_bld.bat VCVER-WIN32 142
chcp 932
cmd /c all_bld.bat VCVER-WIN32 90
cmd /c all_bld.bat VCVER-WIN32 110
cmd /c all_bld.bat VCVER-WIN32 120
cmd /c all_bld.bat BORLAND
chcp 65001
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

:BLD_VCVER_WIN32
set vcver=vc%2
copy ..\toolchain\vc-win32-toolchain.cmake    ..\toolchain\%vcver%-win32-toolchain.cmake
copy ..\toolchain\vc-win32-md-toolchain.cmake ..\toolchain\%vcver%-win32-md-toolchain.cmake
call setcc.bat %vcver% win32
call bld.bat %vcver%-win32
call bld.bat %vcver%-win32-md
del ..\toolchain\%vcver%-*.cmake
goto END

:BLD_WATCOM
call setcc.bat watcom
call bld.bat watcom-win32
call bld.bat watcom-dos32
call bld.bat watcom-dos16-s
call bld.bat watcom-pcat-dos32
call bld.bat watcom-pcat-dos16-s
call bld.bat watcom-pc98-dos16-s
call bld.bat watcom-pc98-dos32
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

:BLD_IA16_ELF_EXE
wsl -d Ubuntu-22.04 bash -c "~/proj/cons/bld/bld.sh ia16-pcat-dos16-s"
goto END

:LINUX
wsl -d Ubuntu-22.04 bash -c "~/proj/cons/bld/bld.sh linux"
goto END

:ERR
@echo Invalid argument : %1
goto END

:END
