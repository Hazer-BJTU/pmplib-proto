#pragma once

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <exception>
#include <iomanip>

#define LENGTH 256
#define ZERO 128
#define BASE 1000
#define LGBASE 3
#define FLIP_SIGN 1
#define HOLD_SIGN 0

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
    BasicNum(bool delayed=false);
    BasicNum(std::string str);
    BasicNum(const BasicNum& num);
    BasicNum& operator = (const BasicNum&) = delete;
    ~BasicNum();
    int operator [] (int idx) const;
    int& operator [] (int idx);
    void initialize();
};

void kernel_add_with_carry(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst);
int kernel_subtraction_with_carry(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst);
void kernel_multiply_interval(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst, int left, int right);
void kernel_multiply_carry(BasicNum* dst);
void flip_sign(real_number_sign& sign);
real_number_sign sign_for_mult(real_number_sign sr1, real_number_sign sr2);
std::ostream& operator << (std::ostream& stream, const BasicNum& sr1);

}
