#include "../include/BasicNum.h"

namespace rpc1k {

BasicNum::BasicNum(bool delayed) {
    sign = POSITIVE;
    if (!delayed) {
        data = (int*)malloc(sizeof(int) * LENGTH);
        if (!data) {
            throw std::bad_alloc();
        }
        memset(data, 0, sizeof(int) * LENGTH);
    }
}

BasicNum::BasicNum(std::string str): BasicNum() {
    int len = str.length();
    if (len != 0) {
        sign = POSITIVE;
        if (str[0] == '+') {
            str = str.substr(1, len - 1);
            len--;
        } else if (str[0] == '-') {
            sign = NEGATIVE;
            str = str.substr(1, len - 1);
            len--;
        }
        int point = str.find('.');
        if (point == std::string::npos) {
            for (int i = len - 1; i >= 0; i--) {
                if (str[i] < '0' || str[i] > '9') {
                    throw std::invalid_argument("Please enter decimal numbers!");
                }
                int bias = len - i - 1, value = str[i] - '0';
                int k = bias / LGBASE, power = bias % LGBASE;
                for (int j = 0; j < power; j++) {
                    value *= 10;
                }
                data[k + ZERO] += value;
            }
        } else {
            for (int i = point - 1; i >= 0; i--) {
                if (str[i] < '0' || str[i] > '9') {
                    throw std::invalid_argument("Please enter decimal numbers!");
                }
                int bias = point - i - 1, value = str[i] - '0';
                int k = bias / LGBASE, power = bias % LGBASE;
                for (int j = 0; j < power; j++) {
                    value *= 10;
                }
                data[ZERO + k] += value;
            }
            for (int i = point + 1; i < len; i++) {
                if (str[i] < '0' || str[i] > '9') {
                    throw std::invalid_argument("Please enter decimal numbers!");
                }
                int bias = i - point - 1, value = str[i] - '0';
                int k = bias / LGBASE, power = LGBASE - bias % LGBASE - 1;
                for (int j = 0; j < power; j++) {
                    value *= 10;
                }
                data[ZERO - k - 1] += value;
            }
        }
    }
    return;
}

BasicNum::BasicNum(const BasicNum& num) {
    sign = num.sign;
    data = (int*)malloc(sizeof(int) * LENGTH);
    if (!data) {
        throw std::bad_alloc();
    }
    memcpy(data, num.data, sizeof(int) * LENGTH);
}

BasicNum::~BasicNum() {
    if (data) {
        free(data);
        data = NULL;
    }
}

int BasicNum::operator [] (int idx) const {
    assert(idx >= 0 && idx < LENGTH);
    return data[idx];
}

int& BasicNum::operator [] (int idx) {
    assert(idx >= 0 && idx < LENGTH);
    return data[idx];
}

void BasicNum::initialize() {
    if (!data) {
        data = (int*)malloc(sizeof(int) * LENGTH);
        if (!data) {
            throw std::bad_alloc();
        }
        memset(data, 0, sizeof(int) * LENGTH);
    }
    return;
}

void kernel_add_with_carry(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst) {
    int carry = 0;
    for (int i = 0; i < LENGTH; i++) {
        dst->data[i] = sr1->data[i] + sr2->data[i] + carry;
        carry = dst->data[i] / BASE;
        dst->data[i] %= BASE;
    }
    return;
}

int kernel_subtraction_with_carry(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst) {
    int carry = 0;
    for (int i = 0; i < LENGTH; i++) {
        dst->data[i] = sr1->data[i] - sr2->data[i] + carry;
        if (dst->data[i] < 0) {
            carry = -1;
            dst->data[i] += BASE;
        } else {
            carry = 0;
        }
    }
    if (dst->data[LENGTH - 1] == BASE - 1) {
        carry = 1;
        for (int i = 0; i < LENGTH; i++) {
            dst->data[i] = BASE - 1 - dst->data[i] + carry;
            if (dst->data[i] == BASE) {
                dst->data[i] = 0;
                carry = 1;
            } else {
                carry = 0;
            }
        }
        return FLIP_SIGN;
    } else {
        return HOLD_SIGN;
    }
}

void kernel_multiply_interval(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst, int left, int right) {
    for (int k = left; k < right; k++) {
        dst->data[k] = 0;
        for (int i = 0; i < LENGTH; i++) {
            int j = k + ZERO - i;
            if (j < 0 || j >= LENGTH) {
                continue;
            }
            dst->data[k] += sr1->data[i] * sr2->data[j];
        }
    }
    return;
}

void kernel_multiply_carry(BasicNum* dst) {
    int carry = 0;
    for (int i = 0; i < LENGTH; i++) {
        dst->data[i] += carry;
        carry = dst->data[i] / BASE;
        dst->data[i] %= BASE;
    }
    return;
}

void flip_sign(real_number_sign& sign) {
    if (sign == POSITIVE) sign = NEGATIVE;
    else sign = POSITIVE;
    return;
}

real_number_sign sign_for_mult(real_number_sign sr1, real_number_sign sr2) {
    if (sr1 == sr2) return POSITIVE;
    else return NEGATIVE;
}

std::ostream& operator << (std::ostream& stream, const BasicNum& sr1) {
    int ptr1 = 0, ptr2 = LENGTH - 1;
    while(sr1.data[ptr2] == 0 && ptr2 > ZERO) {
        ptr2--;
    }
    while(sr1.data[ptr1] == 0 && ptr1 + 1 < ZERO) {
        ptr1++;
    }
    if (sr1.sign == NEGATIVE) stream << '-';
    for (int i = ptr2; i >= ptr1; i--) {
        if (i == ptr2) stream << sr1.data[i];
        else stream << std::setw(LGBASE) << std::setfill('0') << sr1.data[i];
        if (i == ZERO) {
            stream << '.';
        }
    }
    return stream;
}

}
