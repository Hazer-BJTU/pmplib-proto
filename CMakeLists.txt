cmake_minimum_required(VERSION 3.10)

project(RPC-1K VERSION 1.0)

include(CheckCXXCompilerFlag)

set(OPTIMIZATION_FLAGS "-O3")
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    check_cxx_compiler_flag("-mavx2" AVX2_AVAILABLE)
    if (AVX2_AVAILABLE)
        list(APPEND OPTIMIZATION_FLAGS "-mavx2" "-mfma")
        add_definitions(-DAVX2_IN_USE)
    endif()
elseif(MSVC)
    list(APPEND OPTIMIZATION_FLAGS "/arch:AVX2")
    add_definitions(-DAVX2_IN_USE)
endif()

message("Optimization flags: ${OPTIMIZATION_FLAGS}.")
add_compile_options("${OPTIMIZATION_FLAGS}")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB INC LIST_DIRECTORIES True "${PROJECT_SOURCE_DIR}/include/*")
file(GLOB_RECURSE SRC "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE TEST "${PROJECT_SOURCE_DIR}/test/*.cpp")

include_directories(${INC})
add_library(rpc1k SHARED ${SRC})

foreach(test_source_file ${TEST})
    get_filename_component(executable_name ${test_source_file} NAME_WE)
    add_executable(${executable_name} ${test_source_file})
    target_link_libraries(${executable_name} rpc1k)
    set_target_properties(
        ${executable_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
        "${PROJECT_SOURCE_DIR}/bin/tests"
    )
endforeach()
