#pragma once

#include <cstdint>
#include <iostream>

#include "IOBasic.hpp"

namespace mpengine::iofun {

constexpr uint64_t io_base(IOBasic iobasic) noexcept {
    switch(iobasic) {
        case IOBasic::oct: return 8ull;
        case IOBasic::dec: return 10ull;
        case IOBasic::hex: return 16ull;
        default: return 10ull;
    }
}

constexpr uint64_t store_base(IOBasic iobasic) noexcept {
    switch(iobasic) {
        case IOBasic::oct: return 134217728ull;
        case IOBasic::dec: return 100000000ull;
        case IOBasic::hex: return 268435456ull;
        default: return 100000000ull;
    }
}

constexpr uint64_t log_store_base(IOBasic iobasic) noexcept {
    switch(iobasic) {
        case IOBasic::oct: return 9ull;
        case IOBasic::dec: return 8ull;
        case IOBasic::hex: return 7ull;
        default: return 8ull;
    }
}

constexpr const char* base_name(IOBasic iobasic) noexcept {
    switch(iobasic) {
        case IOBasic::oct: return "Oct";
        case IOBasic::dec: return "Dec";
        case IOBasic::hex: return "Hex";
        default: return "Dec";
    }
}

uint64_t digit_parse(const char digit);

size_t precision_to_log_len(const size_t digits_cnt, const IOBasic iobasic) noexcept;

void write_store_digit_to_stream(std::ostream& stream, const IOBasic iobasic, uint64_t digit, bool filling) noexcept;

}
