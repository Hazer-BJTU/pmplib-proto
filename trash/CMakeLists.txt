cmake_minimum_required(VERSION 3.10)

project(RPC-1K VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(SRC
    ${PROJECT_SOURCE_DIR}/src/BasicNum.cpp
    ${PROJECT_SOURCE_DIR}/src/Real.cpp
    ${PROJECT_SOURCE_DIR}/src/CompGraphNodes.cpp
    ${PROJECT_SOURCE_DIR}/src/CompReign.cpp
    ${PROJECT_SOURCE_DIR}/src/ThreadPool.cpp
)

set(HEAD
    ${PROJECT_SOURCE_DIR}/include/TaskHandler
)

add_compile_options(-O2)

add_library(rpc1k SHARED ${SRC})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(example main.cpp)
target_link_libraries(example rpc1k)
