#pragma once

#include <stdlib.h>
#include <string.h>

#define LENGTH 256
#define ZERO 128
#define BASE 1000

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
    ~BasicNum();
    int operator [] (int idx) const;
    int& operator [] (int idx);
};

void kernel_add_with_carry(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst);
void kernel_bare_subtraction(const BasicNum* sr1, const BasicNum* sr2, BasicNum* dst);
void kernel_carry(BasicNum* dst);
void kernel_flip(BasicNum* dst);
