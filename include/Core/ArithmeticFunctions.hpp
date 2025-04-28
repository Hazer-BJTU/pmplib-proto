#pragma once

#include "BaseNum.h"
#include "BaseNumIO.h"
#include <algorithm>

namespace rpc1k {

inline constexpr int64 DEFAULT_ARITHM_BASE = BASE;

inline int arithmetic_numerical_comp(int64* A, int64* B) {
    for (int i = LENGTH - 1; i >= 0; i--) {
        if (A[i] > B[i]) {
            return 1;
        } else if (A[i] < B[i]) {
            return -1;
        }
    }
    return 0;
}

inline bool arithmetic_numerical_add_carry(int64* A, int64* B, int64* C) {
    int64 carry = 0ull;
    for (int i = 0; i < LENGTH; i++) {
        C[i] = A[i] + B[i] + carry;
        carry = C[i] / DEFAULT_ARITHM_BASE;
        C[i] %= DEFAULT_ARITHM_BASE;
    }
    return carry > 0ull;
}

inline bool arithmetic_numerical_sub_carry(int64* A, int64* B, int64* C) {
    int64 carry = 0ull;
    for (int i = 0; i < LENGTH; i++) {
        if (A[i] < (B[i] + carry)) {
            C[i] = A[i] + DEFAULT_ARITHM_BASE - B[i] - carry;
            carry = 1ull;
        } else {
            C[i] = A[i] - B[i] - carry;
            carry = 0ull;
        }
    }
    return carry > 0ull;
}

inline void arithmetic_numerical_multiply(int64* A, int64* B, int64* C, int starting, int ending) {
    for (int i = starting; i < ending; i++) {
        C[i] = 0ull;
        for (int j = std::max(0, i + ZERO - (int)LENGTH + 1); j < std::min(i + ZERO + 1, (int)LENGTH); j++) {
            int k = i + ZERO - j;
            C[i] += A[j] * B[k];
        }
    }
    return;
}

inline bool arithmetic_numerical_carry(int64* A) {
    int64 carry = 0ull;
    for (int i = 0; i < LENGTH; i++) {
        A[i] += carry;
        carry = A[i] / DEFAULT_ARITHM_BASE;
        A[i] %= DEFAULT_ARITHM_BASE;
    }
    return carry > 0ull;
}

}
