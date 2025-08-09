file(GLOB SOURCES "src/*")
file(GLOB TESTS "test/*")

message(STATUS "<core>- core lib option flags used: ${COMPILER_OPTION_FLAGS}")

add_library(core SHARED ${SOURCES})
set_target_properties(core PROPERTIES PREFIX "${PROJECT_NAME}_")
target_compile_definitions(core PRIVATE MPENGINE_CONFIG_LOAD_DEFAULT)
target_link_libraries(core PUBLIC putils) 
target_link_options(core PRIVATE ${COMPILER_OPTION_FLAGS})

message(STATUS "<core>- Start checking macros for core.")
set(MPENGINE_MACROS MPENGINE_CONFIG_LOAD_DEFAULT MPENGINE_FSM_IMPLICIT_OVERLOAD)
foreach (macro ${MPENGINE_MACROS})
    get_target_property(DEFS core COMPILE_DEFINITIONS)
    list(FIND macro ${DEFS} POS)
    if(NOT POS EQUAL -1)
        message(STATUS "<core>- [✓] ${macro}")
    else()
        message(STATUS "<core>- [ ] ${macro}")
    endif()
endforeach()

if(GENERATE_TEST_TARGETS)
    message(STATUS "<core>- Start generating test targets for core.")
    message(STATUS "<core>- core tests option flags used: ${COMPILER_OPTION_FLAGS}")
    foreach (test_source_file ${TESTS})
        get_filename_component(executable_name ${test_source_file} NAME_WE)
        add_executable(${executable_name} ${test_source_file})
        target_link_options(${executable_name} PRIVATE ${COMPILER_OPTION_FLAGS})
        target_link_libraries(${executable_name} core)
        set_target_properties(
            ${executable_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/core_tests"
        )
    endforeach()
else()
    message(STATUS "<core>- Skip generating test targets for core.")
endif()
