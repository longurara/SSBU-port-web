cmake_minimum_required(VERSION 3.20)
project(ssbu_web_maxwell LANGUAGES C CXX)
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_JS OFF CACHE BOOL "" FORCE)
set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)
set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_SPIRV ON CACHE BOOL "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_subdirectory("${ROOT}/deps/glslang-source" glslang)
add_executable(maxwell "${ROOT}/web/maxwell_compiler.c" "${ROOT}/runtime/mx_translate.c")
target_include_directories(maxwell PRIVATE "${ROOT}/runtime" "${ROOT}/deps/glslang-source")
target_link_libraries(maxwell PRIVATE glslang glslang-default-resource-limits SPIRV)
set_target_properties(maxwell PROPERTIES SUFFIX ".mjs" RUNTIME_OUTPUT_DIRECTORY "${ROOT}/build/web-poc")
target_link_options(maxwell PRIVATE --no-entry -O2
  "-sMODULARIZE=1" "-sEXPORT_ES6=1" "-sENVIRONMENT=web,worker,node"
  "-sALLOW_MEMORY_GROWTH=1" "-sINITIAL_MEMORY=33554432" "-sMAXIMUM_MEMORY=536870912"
  "-sSTACK_SIZE=2097152" "-sFILESYSTEM=0"
  "-sEXPORTED_FUNCTIONS=['_malloc','_free','_mx_compile','_mx_error','_mx_glsl','_mx_info','_mx_spirv','_mx_spirv_size']"
  "-sEXPORTED_RUNTIME_METHODS=['HEAPU8','UTF8ToString']")
