#include "./include/Real.h"
#include <iostream>

using namespace rpc1k;

int main() {
    Real r1("10E300"), r2("+"), r3;
    kernel_add_with_carry(
        reinterpret_cast<const BasicNum*>(r1.p.get()),
        reinterpret_cast<const BasicNum*>(r2.p.get()),
        reinterpret_cast<BasicNum*>(r3.p.get())
    );
    std::cout << r1 << std::endl << r2 << std::endl << r3 << std::endl;
    return 0;
}
