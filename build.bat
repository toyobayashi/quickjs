@echo off

if exist build rd /s /q build

set cmk=cmake
set type=Release
set arch=Win32
set toolchain=msvc

:next-arg
if "%1"=="" goto args-done
if /i "%1"=="msvc" set toolchain=msvc&goto arg-ok
if /i "%1"=="mingw" set toolchain=mingw&goto arg-ok
if /i "%1"=="emcc" set toolchain=emcc&set cmk=emcmake cmake&goto arg-ok
if /i "%1"=="Win32" set arch=Win32&goto arg-ok
if /i "%1"=="x64" set arch=x64&goto arg-ok
if /i "%1"=="Debug" set type=Debug&goto arg-ok
if /i "%1"=="Release" set type=Release&goto arg-ok

:arg-ok
shift
goto next-arg

:args-done
if "%toolchain%"=="msvc" goto msvc
if "%toolchain%"=="mingw" goto makefile
if "%toolchain%"=="emcc" goto makefile

:msvc
call %cmk% -A %arch% -H. -Bbuild
cmake --build build --config %type%
goto :EOF

:makefile
call %cmk% ^
  -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=%type% ^
  -DCMAKE_VERBOSE_MAKEFILE=1 ^
  -H. -Bbuild

cmake --build build
goto :EOF
