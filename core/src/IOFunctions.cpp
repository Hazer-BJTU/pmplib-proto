#include "IOFunctions.h"

#include <bit>

#include "GeneralException.h"

namespace mpengine::iofun {

uint64_t digit_parse(const char digit) {
    if ('0' <= digit && digit <= '9') {
        return digit - '0';
    } else if ('A' <= digit && digit <= 'Z') {
        return digit - 'A' + 10ull;
    } else if ('a' <= digit && digit <= 'z') {
        return digit - 'a' + 10ull;
    } else {
        std::stringstream ss;
        ss << "Invalid character in integer: '" << digit << "'!";
        throw PUTILS_GENERAL_EXCEPTION(ss.str(), "parse error");
    }
}

size_t precision_to_log_len(const size_t digits_cnt, const IOBasic iobasic) noexcept {
    return std::countr_zero(std::bit_ceil((digits_cnt + log_store_base(iobasic) - 1) / log_store_base(iobasic)));
}

void write_store_digit_to_stream(std::ostream& stream, const IOBasic iobasic, uint64_t digit, bool filling) noexcept {
    if (filling) {
        stream << std::setw(log_store_base(iobasic)) << std::setfill('0');
    }
    switch(iobasic) {
        case IOBasic::oct: stream << std::oct << digit; return;
        case IOBasic::dec: stream << std::dec << digit; return;
        case IOBasic::hex: stream << std::hex << digit; return;
        default: stream << digit;
    }
    return;
}

}
