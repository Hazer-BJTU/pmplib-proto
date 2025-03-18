#include "../include/BasicNum.h"

namespace rpc1k {

BasicNum::BasicNum() {
    sign = POSITIVE;
    data = (int*)malloc(sizeof(int) * LENGTH);
    if (!data) {
        std::cerr << "Failed to allocate memory!" << std::endl;
        exit(ERROR);
    }
    memset(data, 0, sizeof(int) * LENGTH);
}

BasicNum::BasicNum(const BasicNum& num) {
    assert(this != &num);
    sign = num.sign;
    data = (int*)malloc(sizeof(int) * LENGTH);
    if (!data) {
        std::cerr << "Failed to allocate memory!" << std::endl;
        exit(ERROR);
    }
    memcpy(data, num.data, sizeof(int) * LENGTH);
}

BasicNum& BasicNum::operator = (const BasicNum& num) {
    assert(this != &num);
    sign = num.sign;
    data = (int*)malloc(sizeof(int) * LENGTH);
    if (!data) {
        std::cerr << "Failed to allocate memory!" << std::endl;
        exit(ERROR);
    }
    memcpy(data, num.data, sizeof(int) * LENGTH);
    return *this;
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

void kernel_add_with_carry(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst) {
    int carry = 0;
    for (int i = 0; i < LENGTH; i++) {
        dst->data[i] = sr1->data[i] + sr2->data[i] + carry;
        carry = dst->data[i] / BASE;
        dst->data[i] %= BASE;
    }
    return;
}

void kernel_bare_subtraction(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst) {
    for (int i = 0; i < LENGTH; i++) {
        dst->data[i] = sr1->data[i] - sr2->data[i];
    }
    return;
}

void kernel_carry(BasicNum* dst) {
    int carry = 0;
    for (int i = 0; i < LENGTH; i++) {
        dst->data[i] += carry;
        if (dst->data[i] < 0) {
            //When performing subtraction, dst[i] + BASE must be greater than 0.
            carry = -1;
            dst->data[i] += BASE;
        } else {
            carry = dst->data[i] / BASE;
            dst->data[i] %= BASE;
        }
    }
    return;
}

void kernel_flip(BasicNum* dst) {
    int carry = 1;
    for (int i = 0; i < LENGTH; i++) {
        dst->data[i] = BASE - dst->data[i] - 1;
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

}
