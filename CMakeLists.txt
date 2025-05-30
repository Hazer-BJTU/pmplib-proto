cmake_minimum_required(VERSION 3.20)

project(RPC-1K VERSION 0.1)

include(CheckCXXCompilerFlag)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMPILER_OPTION_FLAGS "-O3" "-g" "-rdynamic")

message("-- Option flags used: ${COMPILER_OPTION_FLAGS}")
add_compile_options(${COMPILER_OPTION_FLAGS})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

include_directories(
    "${PROJECT_SOURCE_DIR}/putils/include"
)

add_subdirectory(putils)
