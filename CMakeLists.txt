cmake_minimum_required(VERSION 3.10)

project(RPC-1K VERSION 1.0)

set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB INC LIST_DIRECTORIES True "${PROJECT_SOURCE_DIR}/include/*")
file(GLOB_RECURSE SRC "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE TEST "${PROJECT_SOURCE_DIR}/test/*.cpp")

add_compile_options(-O3)

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
