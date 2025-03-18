#include "./include/Real.h"
#include <iostream>

using namespace rpc1k;

int main() {
    Real r1("-1049.44572"), r2, r3("5520.79631");
    kernel_add_with_carry(reinterpret_cast<const BasicNum*>(r1.p.get()), reinterpret_cast<const BasicNum*>(r3.p.get()), reinterpret_cast<BasicNum*>(r2.p.get()));
    std::cout << r1 << std::endl;
    std::cout << r3 << std::endl;
    std::cout << r2 << std::endl;
    return 0;
}
