#include "../include/BasicNum.h"

BasicNum::BasicNum() {
    sign = POSITIVE;
    data = (int*)malloc(sizeof(int) * LENGTH);
    memset(data, 0, sizeof(data));
}

BasicNum::~BasicNum() {
    free(data);
}

int BasicNum::operator [] (int idx) const {
    return data[idx];
}

int& BasicNum::operator [] (int idx) {
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
