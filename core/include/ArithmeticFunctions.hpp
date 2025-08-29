#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <algorithm>

namespace mpengine {

using u64arr = uint64_t*;

inline bool u64_variable_length_integer_addition_with_carry(const u64arr a, const u64arr b, u64arr c, const size_t length, const uint64_t base) noexcept {
    uint64_t carry = 0ull;
    for (size_t i = 0; i < length; i++) {
        c[i] = a[i] + b[i] + carry;
        if (c[i] >= base) {
            c[i] = c[i] - base;
            carry = 1ull;
        } else {
            carry = 0ull;
        }
    }
    return carry != 0ull;
}

inline int u64_variable_length_integer_compare(const u64arr a, const u64arr b, const size_t length) noexcept {
    for (size_t i = length - 1; ; i--) {
        if (a[i] > b[i]) {
            return 1;
        } else if (a[i] < b[i]) {
            return -1;
        }
        if (i == 0) {
            break;
        }
    }
    return 0;
}

inline bool u64_variable_length_integer_subtraction_with_carry_a_ge_b(const u64arr a, const u64arr b, u64arr c, const size_t length, const uint64_t base) noexcept {
    uint64_t carry = 0ull;
    for (size_t i = 0; i < length; i++) {
        if (a[i] >= b[i] + carry) {
            c[i] = a[i] - b[i] - carry;
            carry = 0ull;
        } else {
            c[i] = a[i] + base - b[i] - carry;
            carry = 1ull;
        }
    }
    return carry != 0ull;
}

inline bool u64_variable_length_integer_multiplication_c_2len_with_carry(const u64arr a, const u64arr b, u64arr c, const size_t length, const uint64_t base) noexcept {
    //Array c is guraranteed to be filled with zeros.
    for (size_t i = 0; i < length; i++) {
        uint64_t carry = 0ull;
        for (size_t j = 0; j < length; j++) {
            uint64_t total = c[i + j] + a[i] * b[j] + carry;
            if (total >= base) {
                c[i + j] = total % base;
                carry = total / base;
            } else {
                c[i + j] = total;
                carry = 0ull;
            }
        }
        c[i + length] = carry;
    }
    return c[(length << 1) - 1] < base;
}

}
