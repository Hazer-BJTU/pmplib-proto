file(GLOB SOURCES "src/*")
file(GLOB TESTS "test/*")

message(STATUS "putils lib option flags used: ${COMPILER_OPTION_FLAGS}")

add_library(putils SHARED ${SOURCES})
set_target_properties(putils PROPERTIES PREFIX "${PROJECT_NAME}_") 
target_compile_definitions(putils PRIVATE PUTILS_GENERAL_EXCEPTION_KNOWN_ONLY)
target_link_options(putils PRIVATE ${COMPILER_OPTION_FLAGS})

set(PUTILS_MACROS PUTILS_GENERAL_EXCEPTION_KNOWN_ONLY PUTILS_MEMORY_LEAK_CHECK)
foreach (macro ${PUTILS_MACROS})
    get_target_property(DEFS putils COMPILE_DEFINITIONS)
    list(FIND macro ${DEFS} POS)
    if(NOT POS EQUAL -1)
        message(STATUS "[✓] ${macro}")
    else()
        message(STATUS "[ ] ${macro}")
    endif()
endforeach()

message(STATUS "putils tests option flags used: ${COMPILER_OPTION_FLAGS}")

foreach (test_source_file ${TESTS})
    get_filename_component(executable_name ${test_source_file} NAME_WE)
    add_executable(${executable_name} ${test_source_file})
    target_link_options(${executable_name} PRIVATE ${COMPILER_OPTION_FLAGS})
    target_link_libraries(${executable_name} putils)
    set_target_properties(
        ${executable_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests"
    )
endforeach()
