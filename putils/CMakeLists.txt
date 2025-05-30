file(GLOB SOURCES "src/*")
file(GLOB TESTS "test/*")

add_library(putils SHARED "${SOURCES}")

foreach (test_source_file "${TESTS}")
    get_filename_component(executable_name "${test_source_file}" NAME_WE)
    add_executable("${executable_name}" "${test_source_file}")
    target_link_libraries("${executable_name}" putils)
    set_target_properties(
        "${executable_name}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests"
    )
endforeach()
