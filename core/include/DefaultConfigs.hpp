#pragma once

namespace mpengine {

inline constexpr const char* MPENGINE_DEFAULT_CONFIGURATIONS_STRING = 
    "{"
    "    \"Configurations\": {"
    "        <This is where the default configurations begin.>"
    "        <Users should not modify compile-time constant configurations directly.>"
    "        <Instead, import configurations via void read_from(file_path).>"
    "    }"
    "}";
    
}
