cmake_minimum_required(VERSION 3.20)

# project(pmplib VERSION 0.1 LANGUAGES CXX CUDA)
project(pmplib VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_CUPMP ON)
option(GENERATE_TEST_TARGETS OFF)
option(BUILD_RELEASE OFF)

if(BUILD_RELEASE)
    set(CXX_COMPILER_OPTION_FLAGS "-O3")
else()
    set(CXX_COMPILER_OPTION_FLAGS "-g3" "-rdynamic" "-O0")
endif()

message(STATUS "[Global]- Global option flags used: ${CXX_COMPILER_OPTION_FLAGS}")
add_compile_options(${CXX_COMPILER_OPTION_FLAGS})

if(CMAKE_CUDA_COMPILER AND USE_CUPMP)
    # message(STATUS "[Global]- CUDA compiler online.")
    # set(CMAKE_CUDA_STANDARD 20)
    # set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    # find_package(CUDAToolkit REQUIRED)
    set(CUDA_ONLINE ON)
else()
    message(STATUS "[GLobal]- CUDA compiler offline.")
    set(CUDA_ONLINE OFF)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

include_directories(
    "${PROJECT_SOURCE_DIR}/putils/include"
    "${PROJECT_SOURCE_DIR}/core/include"
)

macro(init_compile_definition_option MODULE)
    set(${MODULE}_MACROS "")
    set(${MODULE}_ENABLED_MACROS "")
endmacro()

macro(add_compile_definition_option MODULE DEFINITION_OPTION DEFAULT)
    option(ENABLE_${DEFINITION_OPTION} ${DEFAULT})
    list(APPEND ${MODULE}_MACROS ${DEFINITION_OPTION})
    if (ENABLE_${DEFINITION_OPTION})
        list(APPEND ${MODULE}_ENABLED_MACROS ${DEFINITION_OPTION})
    endif()
endmacro()

add_subdirectory(putils)
add_subdirectory(core)
add_subdirectory(cupmp)
