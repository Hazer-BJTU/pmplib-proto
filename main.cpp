#include "./include/BasicNum.h"
#include <iostream>

int main() {
    BasicNum x, y, z;
    x.data[ZERO + 1] = 445, x.data[ZERO] = 195, x.data[ZERO - 1] = 443;
    y.data[ZERO + 1] = 192, y.data[ZERO] = 101, y.data[ZERO - 1] = 1;
    unsigned_multiply_prec(x, y, z);
    unsigned_calc_carry(z);
    std::cout << z.data[ZERO + 3] << z.data[ZERO + 2] << z.data[ZERO + 1] << z.data[ZERO] << '.' << z.data[ZERO - 1] << z.data[ZERO - 2] << std::endl;
    return 0;
}
