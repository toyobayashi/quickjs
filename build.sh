#!/bin/sh

rm -rf ./build

cmk=cmake

if [ "x$1" == "xemcc" ]; then
  cmk="emcmake cmake"
fi

$cmk -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_VERBOSE_MAKEFILE=1 \
     -H. -Bbuild

cmake --build build
