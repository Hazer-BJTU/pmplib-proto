file(GLOB SOURCES "src/*")
file(GLOB TESTS "test/*")

message(STATUS "core lib option flags used: ${COMPILER_OPTION_FLAGS}")

add_library(core SHARED ${SOURCES})
set_target_properties(core PROPERTIES PREFIX "${PROJECT_NAME}_")
target_compile_definitions(core PRIVATE MPENGINE_CONFIG_LOAD_DEFAULT)
target_link_libraries(core PUBLIC putils) 
target_link_options(core PRIVATE ${COMPILER_OPTION_FLAGS})

set(MPENGINE_MACROS MPENGINE_CONFIG_LOAD_DEFAULT)
foreach (macro ${MPENGINE_MACROS})
    get_target_property(DEFS core COMPILE_DEFINITIONS)
    list(FIND macro ${DEFS} POS)
    if(NOT POS EQUAL -1)
        message(STATUS "[✓] ${macro}")
    else()
        message(STATUS "[ ] ${macro}")
    endif()
endforeach()

message(STATUS "core tests option flags used: ${COMPILER_OPTION_FLAGS}")

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

message(STATUS "")

file(GLOB PRES "pre/*")

add_custom_target(pre_execute_all)

foreach (pre_source_file ${PRES})
    get_filename_component(executable_name ${pre_source_file} NAME_WE)
    add_executable(${executable_name} ${pre_source_file})
    target_link_options(${executable_name} PRIVATE ${COMPILER_OPTION_FLAGS})
    target_link_libraries(${executable_name} core)
    set_target_properties(
        ${executable_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/core_pres"
    )
    add_custom_target( run_${executable_name}
        COMMAND $<TARGET_FILE:${executable_name}>
        COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:${executable_name}>
        DEPENDS ${executable_name}
    )
    add_dependencies(pre_execute_all run_${executable_name})
endforeach()
