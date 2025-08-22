cmake_minimum_required(VERSION 3.20)

project(pmplib VERSION 0.1 LANGUAGES CXX CUDA)

include(CheckCXXCompilerFlag)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GENERATE_TEST_TARGETS OFF)
option(BUILD_RELEASE OFF)

if(BUILD_RELEASE)
    set(COMPILER_OPTION_FLAGS "-O3")
else()
    set(COMPILER_OPTION_FLAGS "-g" "-rdynamic" "-O3")
endif()

message(STATUS "[Global]- Global option flags used: ${COMPILER_OPTION_FLAGS}")
add_compile_options(${COMPILER_OPTION_FLAGS})

if(CMAKE_CUDA_COMPILER)
    find_package(CUDAToolkit REQUIRED)
    message(STATUS "[Global]- CUDA compiler online.")
else()
    message(STATUS "[GLobal]- CUDA compiler offline.]")
endif()


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

include_directories(
    "${PROJECT_SOURCE_DIR}/putils/include"
    "${PROJECT_SOURCE_DIR}/core/include"
)

add_subdirectory(putils)
add_subdirectory(core)
add_subdirectory(cupmp)
