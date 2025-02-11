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

if [ "${Toolchain}" == "mac" ]; then
  # cmake -DCMAKE_TOOLCHAIN_FILE=toolchain/${Toolchain}-toolchain.cmake -B bld/${Toolchain}  .
  cmake -G "Xcode" -DTOOLCHAIN_NAME=mac -DTOOLCHAIN_TARGET_PLATFORM=mac -B bld/${Toolchain} .
  cmake --build bld/${Toolchain} --config Release
else
  cmake -DCMAKE_TOOLCHAIN_FILE=toolchain/${Toolchain}-toolchain.cmake -DCMAKE_BUILD_TYPE=Release -B bld/${Toolchain}  .
  cmake --build bld/${Toolchain}
fi
cmake --install bld/${Toolchain} --prefix ${curdir}/bin/${Toolchain}
