#!/bin/bash

Toolchain=$1
cd $(dirname $0)
cd ..

if [ -z "${Toolchain}" ]; then
  case "$OSTYPE" in
    *darwin*)
      Toolchain=mac
      ;;
    *)
      Toolchain=linux
      ;;
  esac
fi

curdir=$(pwd)
mkdir -p ${curdir}/bld
mkdir -p ${curdir}/bld/${Toolchain}

case ${Toolchain} in
  mac*)
    #cmake -DCMAKE_TOOLCHAIN_FILE=toolchain/${Toolchain}-toolchain.cmake -B bld/${Toolchain}  .
    cmake -G "Xcode" --debug-output -B bld/${Toolchain} .
    cmake --build bld/${Toolchain} --config Release
    ;;
  *)
    cmake -DCMAKE_TOOLCHAIN_FILE=toolchain/${Toolchain}-toolchain.cmake -DCMAKE_BUILD_TYPE=Release -B bld/${Toolchain}  .
    cmake --build bld/${Toolchain}
    ;;
esac

cmake --install bld/${Toolchain} --prefix ${curdir}/bin/${Toolchain}
