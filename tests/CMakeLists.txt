cmake_minimum_required(VERSION 3.15)

project(bjson)

if(MSVC)
  add_library(bjson SHARED "bjson.c")
  set_target_properties(${QJS_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  target_link_libraries(bjson PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../build/Release/qjs.lib")
  target_compile_definitions(bjson PRIVATE "JS_SHARED_LIBRARY")
endif()
