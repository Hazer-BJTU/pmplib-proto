#pragma once

#include <string_view>

namespace mpengine {

inline constexpr std::string_view MPENGINE_DEFAULT_CONFIGURATIONS_STRING = 
    "{                                                                                                          "
    "    \"Configurations\": {                                                                                  "
    "        \"_comments\": \"This is where the default configurations begin.                                   "
    "        \n               Users should not modify compile-time constant configurations directly.            "
    "        \n               Instead, import configurations via void read_from(file_path).\"                   "
    "    }                                                                                                      "
    "}                                                                                                          ";
    
}
