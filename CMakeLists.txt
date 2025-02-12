cmake_minimum_required(VERSION 3.15)

project(quickjs)

include(CMakeDependentOption)

option(QJS_STATIC_VCRT "QJS_STATIC_VCRT" ON)
option(QJS_CONFIG_BIGNUM "QJS_CONFIG_BIGNUM" ON)
option(QJS_CONFIG_CHECK_JSVALUE "QJS_CONFIG_CHECK_JSVALUE" OFF)
cmake_dependent_option(QJS_CONFIG_LTO "QJS_CONFIG_LTO" ON "NOT EMSCRIPTEN;NOT MSVC" OFF)
cmake_dependent_option(QJS_CONFIG_ATOMICS "QJS_CONFIG_ATOMICS" ON "NOT EMSCRIPTEN" OFF)
cmake_dependent_option(QJS_USE_WORKER "QJS_USE_WORKER" ON "NOT WIN32;NOT EMSCRIPTEN" OFF)

if(QJS_CONFIG_LTO)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_AR "llvm-ar")
    set(CMAKE_RANLIB "llvm-ranlib")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_AR "gcc-ar")
    set(CMAKE_RANLIB "gcc-ranlib")
  endif()
endif()

set(QJS_LIB_NAME ${PROJECT_NAME})
set(QJS_LIB_LTO_NAME "${PROJECT_NAME}.lto")
set(QJSC_NAME qjsc)
set(QJS_NAME qjs)

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

set(QJS_VERSION "2021-03-27")
if(WIN32 AND NOT MSVC)
  add_compile_definitions("__USE_MINGW_ANSI_STDIO")
# else()
#   execute_process(COMMAND cat VERSION
#                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#                   OUTPUT_VARIABLE QJS_VERSION)
#   string(REGEX REPLACE "\r?\n" "" QJS_VERSION ${QJS_VERSION})
endif()

if(NOT MSVC)
add_compile_definitions(
  "_GNU_SOURCE"
)
else()
set(QJS_LIB_SRC ${QJS_LIB_SRC} "${CMAKE_CURRENT_SOURCE_DIR}/win/pthread.c")
endif()

add_compile_definitions(
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
if(MSVC)
  target_link_options(${QJS_LIB_NAME} PUBLIC
    "legacy_stdio_float_rounding.obj"
  )
  if(QJS_STATIC_VCRT)
    set_target_properties(${QJS_LIB_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
endif()

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
    target_compile_options(${QJS_LIB_NAME} PUBLIC "-pthread")
    target_link_options(${QJS_LIB_NAME} INTERFACE "-pthread")

    if(QJS_CONFIG_LTO)
      target_compile_options(${QJS_LIB_LTO_NAME} PUBLIC "-pthread")
      target_link_options(${QJS_LIB_LTO_NAME} INTERFACE "-pthread")
    endif()
  endif()
endif()

if(NOT EMSCRIPTEN)
  if(WIN32)
    if(NOT MSVC)
      set(QJS_EXE_LINK_LIBS m pthread)
    else()
      target_include_directories(${QJS_LIB_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/win")
      target_compile_options(${QJS_LIB_NAME} PRIVATE "/experimental:c11atomics" "/std:c11")
    endif()
  else()
    set(QJS_EXE_LINK_LIBS m dl pthread)
  endif()

  add_executable(${QJSC_NAME} qjsc.c)
  if(MSVC AND QJS_STATIC_VCRT)
    set_target_properties(${QJSC_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
  if(MINGW)
    target_link_options(${QJSC_NAME} PRIVATE "--static")
  endif()

  target_compile_definitions(${QJSC_NAME} PRIVATE
    "CONFIG_CC=\"${CMAKE_C_COMPILER}\""
    "CONFIG_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
  if(QJS_CONFIG_LTO)
    target_compile_definitions(${QJSC_NAME} PRIVATE
      "CONFIG_LTO")
    target_link_libraries(${QJSC_NAME} ${QJS_LIB_LTO_NAME} ${QJS_EXE_LINK_LIBS})
  else()
    target_link_libraries(${QJSC_NAME} ${QJS_LIB_NAME} ${QJS_EXE_LINK_LIBS})
  endif()

  add_custom_command(OUTPUT repl.c
    COMMAND ${QJSC_NAME} -c -o ${CMAKE_CURRENT_BINARY_DIR}/repl.c -m ${CMAKE_CURRENT_SOURCE_DIR}/repl.js
    DEPENDS ${QJSC_NAME}
    VERBATIM)

  if(QJS_CONFIG_BIGNUM)
    add_custom_command(OUTPUT qjscalc.c
      COMMAND ${QJSC_NAME} -fbignum -c -o ${CMAKE_CURRENT_BINARY_DIR}/qjscalc.c ${CMAKE_CURRENT_SOURCE_DIR}/qjscalc.js
      DEPENDS ${QJSC_NAME}
      VERBATIM)
  endif()

  if(QJS_CONFIG_BIGNUM)
    add_executable(${QJS_NAME} qjs.c repl.c qjscalc.c)
  else()
    add_executable(${QJS_NAME} qjs.c repl.c)
  endif()
  set_target_properties(${QJS_NAME} PROPERTIES ENABLE_EXPORTS 1)
  if(MSVC AND QJS_STATIC_VCRT)
    set_target_properties(${QJS_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  elseif(MINGW)
    target_link_options(${QJS_NAME} PRIVATE "--static")
  endif()

  if(NOT WIN32)
    target_link_options(${QJS_NAME} PRIVATE "-rdynamic")
  endif()
  target_link_libraries(${QJS_NAME} ${QJS_LIB_NAME} ${QJS_EXE_LINK_LIBS})

  install(TARGETS ${QJSC_NAME} DESTINATION "bin")
  install(TARGETS ${QJS_NAME} DESTINATION "bin")

  if(NOT WIN32)
    install(CODE "execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink \"./${QJS_NAME}\" \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin/qjscalc\")")
  endif()
endif()

add_library(bjson SHARED "tests/bjson.c")
if(MSVC AND QJS_STATIC_VCRT)
  set_target_properties(bjson PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
set_target_properties(bjson PROPERTIES PREFIX "")
set_target_properties(bjson PROPERTIES SUFFIX ".qjs")
target_link_libraries(bjson PRIVATE ${QJS_NAME})
target_compile_definitions(bjson PRIVATE "JS_SHARED_LIBRARY")
add_custom_command(TARGET bjson POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:bjson>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/$<TARGET_FILE_NAME:bjson>"
)

install(TARGETS ${QJS_LIB_NAME} DESTINATION "lib/${PROJECT_NAME}")
# install(TARGETS ${QJS_LIB_NAME} DESTINATION "lib/${CMAKE_LIBRARY_ARCHITECTURE}")
if(QJS_CONFIG_LTO)
  install(TARGETS ${QJS_LIB_LTO_NAME} DESTINATION "lib/${PROJECT_NAME}")
  # install(TARGETS ${QJS_LIB_LTO_NAME} DESTINATION "lib/${CMAKE_LIBRARY_ARCHITECTURE}")
endif()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/quickjs.h ${CMAKE_CURRENT_SOURCE_DIR}/quickjs-libc.h
        DESTINATION "include/${PROJECT_NAME}")
