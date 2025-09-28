#include <vector>
#include <iomanip>
#include <chrono>

#include "pmp/integer.h"

int main() {

    auto start = std::chrono::high_resolution_clock::now();

    pmp::context context(7000, pmp::io::hex);
    pmp::integer a_n_2("0", context);
    pmp::integer a_n_1("1", context);
    for (size_t i = 2; i <= 20000; i++) {
        pmp::integer a_n = a_n_1 + a_n_2;
        a_n_2 = a_n_1;
        a_n_1 = a_n;
    }
    std::cout << "(F[20000])hex = 0x" << a_n_1 << std::endl;

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Test time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    return 0;
}