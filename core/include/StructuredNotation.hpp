/**
 * @file Structured notation generation utilities
 * @brief Provides thread-local structured notation builder for JSON-like output generation
 * 
 * @namespace mpengine::stn
 * @brief Structured Notation namespace for generating hierarchical data formats
 */

#pragma once

#include <sstream>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <type_traits>

namespace mpengine::stn {

/**
 * @brief Thread-local context for building structured notation output
 * 
 * Maintains the current state of notation generation including:
 * - Comma placement tracking for proper JSON syntax
 * - Indentation level for pretty-printing
 * - Output string stream for accumulating the generated notation
 */

struct StructuredNotation {
    bool comma_flag;
    std::string indent;
    std::ostringstream oss;
};

inline thread_local StructuredNotation structured_notation;

inline void beg_notation() noexcept {
    structured_notation.comma_flag = false;
    structured_notation.indent = "";
    structured_notation.oss.clear();
    structured_notation.oss.str("");
    structured_notation.oss << '{';
    structured_notation.indent.push_back('\t');
    return;
}

inline void end_notation(std::ostream& stream) noexcept {
    structured_notation.oss << std::endl;
    structured_notation.oss << '}' << std::endl;
    stream << structured_notation.oss.str();
    return;
}

inline void beg_field(std::string_view name) noexcept {
    structured_notation.oss << (structured_notation.comma_flag ? "," : "") << std::endl;
    structured_notation.oss << structured_notation.indent;
    structured_notation.oss << std::quoted(name) << ": {";
    structured_notation.indent.push_back('\t');
    structured_notation.comma_flag = false;
    return;
}

inline void beg_field() noexcept {
    structured_notation.oss << (structured_notation.comma_flag ? "," : "") << std::endl;
    structured_notation.oss << structured_notation.indent;
    structured_notation.oss << '{';
    structured_notation.indent.push_back('\t');
    structured_notation.comma_flag = false;
    return;
}

inline void end_field() noexcept {
    structured_notation.oss << std::endl;
    structured_notation.indent.pop_back();
    structured_notation.oss << structured_notation.indent;
    structured_notation.oss << '}';
    structured_notation.comma_flag = true;
    return;
}

inline void beg_list(std::string_view name) noexcept {
    structured_notation.oss << (structured_notation.comma_flag ? "," : "") << std::endl;
    structured_notation.oss << structured_notation.indent;
    structured_notation.oss << std::quoted(name) << ": [";
    structured_notation.indent.push_back('\t');
    structured_notation.comma_flag = false;
    return;
}

inline void beg_list() noexcept {
    structured_notation.oss << (structured_notation.comma_flag ? "," : "") << std::endl;
    structured_notation.oss << structured_notation.indent;
    structured_notation.oss << '[';
    structured_notation.indent.push_back('\t');
    structured_notation.comma_flag = false;
    return;
}

inline void end_list() noexcept {
    structured_notation.oss << std::endl;
    structured_notation.indent.pop_back();
    structured_notation.oss << structured_notation.indent;
    structured_notation.oss << ']';
    structured_notation.comma_flag = true;
    return;
}

template<typename EntryType>
void entry(std::string_view key, EntryType value) noexcept {
    structured_notation.oss << (structured_notation.comma_flag ? "," : "") << std::endl;
    structured_notation.oss << structured_notation.indent;
    structured_notation.oss << std::quoted(key) << ": ";
    if constexpr (std::is_convertible_v<EntryType, std::string_view>) {
        structured_notation.oss << std::quoted(value);
    } else if constexpr (std::is_same_v<EntryType, bool>) {
        structured_notation.oss << (value ? "true" : "false");
    } else {
        structured_notation.oss << value;
    }
    structured_notation.comma_flag = true;
    return;
}

template<typename EntryType>
void entry(EntryType value) noexcept {
    structured_notation.oss << (structured_notation.comma_flag ? "," : "") << std::endl;
    structured_notation.oss << structured_notation.indent;
    if constexpr (std::is_convertible_v<EntryType, std::string_view>) {
        structured_notation.oss << std::quoted(value);
    } else if constexpr (std::is_same_v<EntryType, bool>) {
        structured_notation.oss << (value ? "true" : "false");
    } else {
        structured_notation.oss << value;
    }
    structured_notation.comma_flag = true;
    return;
}

}

