file(GLOB SOURCES "src/*")
file(GLOB TESTS "test/*")

message(STATUS "<core>- core lib option flags used: ${COMPILER_OPTION_FLAGS}")

set(MPENGINE_MACROS "")
set(MPENGINE_ENABLED_MACROS "")
option(ENABLE_MPENGINE_CONFIG_LOAD_DEFAULT ON)
list(APPEND MPENGINE_MACROS MPENGINE_CONFIG_LOAD_DEFAULT)
if (ENABLE_MPENGINE_CONFIG_LOAD_DEFAULT)
    list(APPEND MPENGINE_ENABLED_MACROS MPENGINE_CONFIG_LOAD_DEFAULT)
endif()
option(ENABLE_MPENGINE_FSM_IMPLICIT_OVERLOAD OFF)
list(APPEND MPENGINE_MACROS MPENGINE_FSM_IMPLICIT_OVERLOAD)
if (ENABLE_MPENGINE_FSM_IMPLICIT_OVERLOAD)
    list(APPEND MPENGINE_ENABLED_MACROS MPENGINE_FSM_IMPLICIT_OVERLOAD)
endif()
option(ENABLE_MPENGINE_THREAD_BINDING_OPTIMIZATION ON)
list(APPEND MPENGINE_MACROS MPENGINE_THREAD_BINDING_OPTIMIZATION)
if (ENABLE_MPENGINE_THREAD_BINDING_OPTIMIZATION)
    list(APPEND MPENGINE_ENABLED_MACROS MPENGINE_THREAD_BINDING_OPTIMIZATION)
endif()

add_library(core SHARED ${SOURCES})
set_target_properties(core PROPERTIES PREFIX "${PROJECT_NAME}_")
target_compile_definitions(core PUBLIC ${MPENGINE_ENABLED_MACROS})
target_link_libraries(core PUBLIC putils) 
target_link_options(core PRIVATE ${COMPILER_OPTION_FLAGS})

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

message(STATUS "<core>- Start post-compile operations for core.")
message(STATUS "<core>- core post-compile operations flags used: ${COMPILER_OPTION_FLAGS}")

add_executable(print_default_configs "post/printdefaultconfigs.cpp")
target_link_options(print_default_configs PRIVATE ${COMPILER_OPTION_FLAGS})
target_link_libraries(print_default_configs core)
set_target_properties(
    print_default_configs PROPERTIES RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/core_post"
)

message(STATUS "<core>- Export default configurations to ${PROJECT_SOURCE_DIR}/config/configurations.conf")

add_custom_target(core_export_default_configs ALL
    DEPENDS print_default_configs
    COMMAND $<TARGET_FILE:print_default_configs> "${PROJECT_SOURCE_DIR}/config/configurations.conf"
)
