#include "../include/BasicNum.h"

BasicNum::BasicNum() {
    sign = POSITIVE;
    data = (int*)malloc(sizeof(int) * LENGTH);
    memset(data, 0, sizeof(data));
}

BasicNum::~BasicNum() {
    free(data);
}

void unsigned_addition(const BasicNum& sr1, const BasicNum& sr2, BasicNum& dst) {
    int carry = 0;
    for (int i = 0; i < LENGTH; i++) {
        dst.data[i] = sr1.data[i] + sr2.data[i] + carry;
        carry = dst.data[i] / BASE;
        dst.data[i] %= BASE;
    }
    return;
}

void unsigned_multiply_prec(const BasicNum& sr1, const BasicNum& sr2, BasicNum& dst) {
    memset(dst.data, 0, sizeof(dst.data));
    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < LENGTH; j++) {
            int k = i + j - ZERO;
            if (k < 0 || k >= LENGTH) {
                continue;
            }
            dst.data[k] += sr1.data[i] * sr2.data[j];
        }
    }
    return;
}

void unsigned_calc_carry(BasicNum& dst) {
    int carry = 0;
    for (int i = 0; i < LENGTH; i++) {
        dst.data[i] += carry;
        carry = dst.data[i] / BASE;
        dst.data[i] %= BASE;
    }
    return;
}
