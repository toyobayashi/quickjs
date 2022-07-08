cmake_minimum_required(VERSION 3.13)

project(quickjs)

include(CMakeDependentOption)

option(QJS_CONFIG_BIGNUM "QJS_CONFIG_BIGNUM" ON)
option(QJS_CONFIG_CHECK_JSVALUE "QJS_CONFIG_CHECK_JSVALUE" OFF)
cmake_dependent_option(QJS_CONFIG_LTO "QJS_CONFIG_LTO" ON "NOT EMSCRIPTEN" OFF)
cmake_dependent_option(QJS_CONFIG_ATOMICS "QJS_CONFIG_ATOMICS" ON "NOT EMSCRIPTEN" OFF)
cmake_dependent_option(QJS_USE_WORKER "QJS_USE_WORKER" ON "NOT WIN32;NOT EMSCRIPTEN" OFF)

set(QJS_LIB_NAME quickjs)
set(QJS_LIB_LTO_NAME quickjs.lto)
set(QJSC_NAME qjsc)

set(QJS_LIB_SRC
  cutils.c
  quickjs.c
  libregexp.c
  libunicode.c
  quickjs-libc.c
)

if(QJS_CONFIG_BIGNUM)
  set(QJS_LIB_SRC ${QJS_LIB_SRC} libbf.c)
endif()

if(WIN32)
  execute_process(COMMAND type VERSION
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  OUTPUT_VARIABLE QJS_VERSION)
else()
  execute_process(COMMAND cat VERSION
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  OUTPUT_VARIABLE QJS_VERSION)
endif()

string(REGEX REPLACE "\r?\n" "" QJS_VERSION ${QJS_VERSION})

add_compile_definitions(
  "_GNU_SOURCE"
  "CONFIG_VERSION=\"${QJS_VERSION}\""
)

if(QJS_CONFIG_BIGNUM)
  add_compile_definitions("CONFIG_BIGNUM")
endif()

if(QJS_USE_WORKER)
  add_compile_definitions("USE_WORKER")
endif()

if(QJS_CONFIG_ATOMICS)
  add_compile_definitions("CONFIG_ATOMICS")
endif()

add_library(${QJS_LIB_NAME} STATIC ${QJS_LIB_SRC})
if(QJS_CONFIG_CHECK_JSVALUE)
  target_compile_definitions(${QJS_LIB_NAME} PUBLIC "CONFIG_CHECK_JSVALUE")
endif()

if(QJS_CONFIG_LTO)
  add_compile_options("-flto")
  add_link_options("-flto")
  add_library(${QJS_LIB_LTO_NAME} STATIC ${QJS_LIB_SRC})
  if(QJS_CONFIG_CHECK_JSVALUE)
    target_compile_definitions(${QJS_LIB_LTO_NAME} PUBLIC "CONFIG_CHECK_JSVALUE")
  endif()
endif()

if(EMSCRIPTEN)
  if(QJS_CONFIG_ATOMICS OR QJS_USE_WORKER)
    target_compile_options(${QJS_LIB_NAME} PUBLIC "-sUSE_PTHREADS=1")
    target_link_options(${QJS_LIB_NAME} INTERFACE "-sUSE_PTHREADS=1")

    if(QJS_CONFIG_LTO)
      target_compile_options(${QJS_LIB_LTO_NAME} PUBLIC "-sUSE_PTHREADS=1")
      target_link_options(${QJS_LIB_LTO_NAME} INTERFACE "-sUSE_PTHREADS=1")
    endif()
  endif()
endif()

install(TARGETS ${QJS_LIB_NAME} DESTINATION "lib/${CMAKE_LIBRARY_ARCHITECTURE}")
install(TARGETS ${QJS_LIB_NAME} DESTINATION "lib/quickjs")
if(QJS_CONFIG_LTO)
  install(TARGETS ${QJS_LIB_LTO_NAME} DESTINATION "lib/quickjs")
  install(TARGETS ${QJS_LIB_LTO_NAME} DESTINATION "lib/${CMAKE_LIBRARY_ARCHITECTURE}")
endif()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/quickjs.h ${CMAKE_CURRENT_SOURCE_DIR}/quickjs-libc.h
        DESTINATION "include/quickjs")
