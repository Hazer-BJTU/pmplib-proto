#include "pmp/integer.h"

int main() {
    pmp::context context(1000, pmp::io::hex);
    pmp::integer a("123400", context), b("-0", context), c("444551234", context);
    pmp::integer d = a, e = d;
    std::cout << a << std::endl;
    std::cout << b << std::endl;
    std::cout << c << std::endl;
    std::cout << d << std::endl;
    std::cout << e << std::endl;
    return 0;
}