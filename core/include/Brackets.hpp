#pragma once

#include "GlobalConfig.h"

#define MPENGINE_DOMAIN_BEG            \
do {                                   \
    mpengine::GlobalConfig tree(true); \
    std::string key_path = "";         \

#define MPENGINE_DOMAIN_END \
} while(0);                 \

#define MD_FIELD_BEG(FIELD_NAME)             \
if (key_path == "") {                        \
    key_path.append(FIELD_NAME);             \
} else {                                     \
    key_path.append("/").append(FIELD_NAME); \
}                                            \

#define MD_FIELD_END                                           \
if (key_path.find_last_of("/") != std::string::npos) {         \
    key_path = key_path.substr(0, key_path.find_last_of("/")); \
}                                                              \

#define MD_FIELD_KVP(KEY, VALUE)  \
MD_FIELD_BEG(KEY)                 \
    tree.insert(key_path, VALUE); \
MD_FIELD_END                      \

#define MD_WRITE_STREAM(STREAM, INDENT)              \
tree.recursive_write("", tree.root, STREAM, INDENT); \

