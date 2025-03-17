#pragma once

#include <stdlib.h>
#include <string.h>

#define LENGTH 256
#define ZERO 128
#define BASE 1000

enum real_number_sign {POSITIVE, NEGATIVE};

/*
    This is the basic number class, which implements the basic operations,
    for example, the unsigned addition and multiplication operations.
    Also, The implementation of these classes uses manual memory management, 
    so it is not recommended for anyone to use them directly 
    in order to avoid potential unknown errors.
    These functionalities are implemented in both a single-threaded 
    way and a parallel computing way. 
    If you need to know more, please refer to CompManager.cpp.
*/
class BasicNum {
public:
    real_number_sign sign;
    int* data;
    BasicNum();
    ~BasicNum();
};

void unsigned_addition(const BasicNum& sr1, const BasicNum& sr2, BasicNum& dst);
void unsigned_multiply_prec(const BasicNum& sr1, const BasicNum& sr2, BasicNum& dst);
void unsigned_calc_carry(BasicNum& dst);
