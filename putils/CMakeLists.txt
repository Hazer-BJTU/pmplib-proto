file(GLOB SOURCES "src/*")
file(GLOB TESTS "test/*")

message(STATUS "<putils>- putils lib option flags used: ${CXX_COMPILER_OPTION_FLAGS}")

init_compile_definition_option(PUTILS)
add_compile_definition_option(PUTILS PUTILS_GENERAL_EXCEPTION_KNOWN_ONLY ON)
add_compile_definition_option(PUTILS PUTILS_MEMORY_LEAK_CHECK OFF)
add_compile_definition_option(PUTILS PUTILS_THREADPOOL_WORKSTEALING_OPTIMIZATION ON)
add_compile_definition_option(PUTILS PUTILS_DIRECT_MEMORY_MAPPER ON)

add_library(putils SHARED ${SOURCES})
set_target_properties(putils PROPERTIES PREFIX "${PROJECT_NAME}_") 
target_compile_definitions(putils PUBLIC ${PUTILS_ENABLED_MACROS})
target_compile_options(putils PUBLIC ${CXX_COMPILER_OPTION_FLAGS})

message(STATUS "<putils>- Start checking macros for putils.")
foreach (macro ${PUTILS_MACROS})
    get_target_property(DEFS putils COMPILE_DEFINITIONS)
    list(FIND DEFS ${macro} POS)
    if(NOT POS EQUAL -1)
        message(STATUS "<putils>- [✓] ${macro}")
    else()
        message(STATUS "<putils>- [ ] ${macro}")
    endif()
endforeach()

if(GENERATE_TEST_TARGETS)
    message(STATUS "<putils>- Start generating test targets for putils.")
    message(STATUS "<putils>- putils tests option flags used: ${CXX_COMPILER_OPTION_FLAGS}")
    foreach (test_source_file ${TESTS})
        get_filename_component(executable_name ${test_source_file} NAME_WE)
        add_executable(${executable_name} ${test_source_file})
        target_compile_options(${executable_name} PRIVATE ${CXX_COMPILER_OPTION_FLAGS})
        target_link_libraries(${executable_name} putils)
        set_target_properties(
            ${executable_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/putils_tests"
        )
    endforeach()
else()
    message(STATUS "<putils>- Skip generating test targets for putils.")
endif()
