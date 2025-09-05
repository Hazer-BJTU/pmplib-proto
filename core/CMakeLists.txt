file(GLOB SOURCES "src/*")
file(GLOB TESTS "test/*")

message(STATUS "<core>- core lib option flags used: ${CXX_COMPILER_OPTION_FLAGS}")

init_compile_definition_option(MPENGINE)
add_compile_definition_option(MPENGINE MPENGINE_CONFIG_LOAD_DEFAULT ON)
add_compile_definition_option(MPENGINE MPENGINE_FSM_IMPLICIT_OVERLOAD OFF)
add_compile_definition_option(MPENGINE MPENGINE_THREAD_BINDING_OPTIMIZATION ON)
add_compile_definition_option(MPENGINE MPENGINE_GRAPHV_DEBUG_OPTION OFF)
add_compile_definition_option(MPENGINE MPENGINE_STORE_PROCEDURE_DETAILS OFF)

add_library(core SHARED ${SOURCES})
set_target_properties(core PROPERTIES PREFIX "${PROJECT_NAME}_")
target_compile_definitions(core PUBLIC ${MPENGINE_ENABLED_MACROS})
target_compile_definitions(core PRIVATE MPENGINE_DEFAULT_CONFIG_PATH="${PROJECT_SOURCE_DIR}/config/configurations.conf")
target_compile_options(core PUBLIC ${CXX_COMPILER_OPTION_FLAGS})
target_link_libraries(core PUBLIC putils) 

message(STATUS "<core>- Start checking macros for core.")
foreach (macro ${MPENGINE_MACROS})
    get_target_property(DEFS core COMPILE_DEFINITIONS)
    list(FIND DEFS ${macro} POS)
    if(NOT POS EQUAL -1)
        message(STATUS "<core>- [✓] ${macro}")
    else()
        message(STATUS "<core>- [ ] ${macro}")
    endif()
endforeach()

if(GENERATE_TEST_TARGETS)
    message(STATUS "<core>- Start generating test targets for core.")
    message(STATUS "<core>- core tests option flags used: ${CXX_COMPILER_OPTION_FLAGS}")
    foreach (test_source_file ${TESTS})
        get_filename_component(executable_name ${test_source_file} NAME_WE)
        add_executable(${executable_name} ${test_source_file})
        target_compile_options(${executable_name} PRIVATE ${CXX_COMPILER_OPTION_FLAGS})
        target_link_libraries(${executable_name} core)
        set_target_properties(
            ${executable_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/core_tests"
        )
    endforeach()
else()
    message(STATUS "<core>- Skip generating test targets for core.")
endif()

# message(STATUS "<core>- Start post-compile operations for core.")
# message(STATUS "<core>- core post-compile operations flags used: ${CXX_COMPILER_OPTION_FLAGS}")
#
# add_executable(print_default_configs "post/printdefaultconfigs.cpp")
# target_compile_options(print_default_configs PRIVATE ${CXX_COMPILER_OPTION_FLAGS})
# target_link_libraries(print_default_configs core)
# set_target_properties(
#     print_default_configs PROPERTIES RUNTIME_OUTPUT_DIRECTORY
#     "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/core_post"
# )
#
message(STATUS "<core>- Default configurations path: ${PROJECT_SOURCE_DIR}/config/configurations.conf.")
#
# add_custom_target(core_export_default_configs ALL
#     DEPENDS print_default_configs
#     COMMAND $<TARGET_FILE:print_default_configs> "${PROJECT_SOURCE_DIR}/config/configurations.conf"
# )
