#include "./include/Real.h"
#include <iostream>

using namespace rpc1k;

int main() {
    Real r1("0.49");
    Real r2("0.01");
    Real r3;
    kernel_multiply_interval(
        reinterpret_cast<const BasicNum*>(r1.p.get()),
        reinterpret_cast<const BasicNum*>(r2.p.get()),
        reinterpret_cast<BasicNum*>(r3.p.get()),
        0,
        LENGTH
    );
    kernel_multiply_carry(reinterpret_cast<BasicNum*>(r3.p.get()));
    std::cout << r3 << std::endl;
    return 0;
}
