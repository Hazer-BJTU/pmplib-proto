#pragma once

#include <string_view>

namespace mpengine {

inline constexpr std::string_view MPENGINE_DEFAULT_CONFIGURATIONS_STRING = 
    "{                                                                                                          "
    "    \"Configurations\": {                                                                                  "
    "        \"_comments\": \"This is where the default configurations begin.                                   "
    "\n                       Users should not modify compile-time constant configurations directly.            "
    "\n                       Instead, import configurations via function void read_from(file_path).\",         "
    "        \"core\": {                                                                                        "
    "            \"BasicIntegerType\": {                                                                        "
    "                \"_comments\": \"Configurations of the BasicIntegerType.\",                                "
    "                \"limits\": { \"min_log_length\": 8, \"max_log_length\": 20 }                              "
    "            }                                                                                              "
    "        }                                                                                                  "
    "    }                                                                                                      "
    "}                                                                                                          ";
    
}
