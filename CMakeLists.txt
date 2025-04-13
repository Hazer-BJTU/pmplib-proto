cmake_minimum_required(VERSION 3.10)

project(RPC-1K VERSION 1.0)

set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(
    INC
    ${PROJECT_SOURCE_DIR}/include/TaskHandler
)
set(
    SRC
    ${PROJECT_SOURCE_DIR}/src/TaskHandler/ThreadPool.cpp
    ${PROJECT_SOURCE_DIR}/src/TaskHandler/Log.cpp
)
set(
    TEST
    ${PROJECT_SOURCE_DIR}/test/TaskHandler/logtest.cpp
)

add_compile_options(-O2)

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
