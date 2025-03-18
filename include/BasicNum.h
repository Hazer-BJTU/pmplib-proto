#pragma once

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define LENGTH 256
#define ZERO 128
#define BASE 1000

#define ERROR -1

namespace rpc1k {

enum real_number_sign {POSITIVE, NEGATIVE};

/*
    This is the basic number class which also implements the kernel functions.
    The implementation of these classes uses manual memory management, 
    so it is not recommended for anyone to use them directly 
    in order to avoid potential errors.
*/
class BasicNum {
public:
    real_number_sign sign;
    int* data;
    BasicNum();
    BasicNum(const BasicNum& num);
    BasicNum& operator = (const BasicNum& num);
    ~BasicNum();
    int operator [] (int idx) const;
    int& operator [] (int idx);
};

void kernel_add_with_carry(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst);
void kernel_bare_subtraction(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst);
void kernel_carry(BasicNum* dst);
void kernel_flip(BasicNum* dst);
void flip_sign(real_number_sign& sign);
real_number_sign sign_for_mult(real_number_sign sr1, real_number_sign sr2);

}
