cmake_minimum_required(VERSION 3.10)

project(RPC-1K VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SRC 
    ${PROJECT_SOURCE_DIR}/src/BasicNum.cpp
    ${PROJECT_SOURCE_DIR}/src/Real.cpp
    ${PROJECT_SOURCE_DIR}/main.cpp
)

add_executable(example ${SRC})
