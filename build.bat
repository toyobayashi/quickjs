@echo off

if exist build rd /s /q build

set cmk=cmake

if /i "%1"=="emcc" set cmk=emcmake cmake

call %cmk% ^
  -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_VERBOSE_MAKEFILE=1 ^
  -H. -Bbuild

cmake --build build
