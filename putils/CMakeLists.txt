file(GLOB SOURCES "src/*")
file(GLOB TESTS "test/*")

message("-- putils option flags used: ${COMPILER_OPTION_FLAGS}")

add_library(putils SHARED ${SOURCES})
set_target_properties(putils PROPERTIES ENABLE_EXPORTS ON)
target_link_options(putils PRIVATE ${COMPILER_OPTION_FLAGS})

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
