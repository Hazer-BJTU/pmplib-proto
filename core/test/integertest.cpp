#include <iostream>
#include "mpengine/integer.h"

int main() {
    size_t log_len = pmp::precision(1000, pmp::io::hex);
    std::cout << log_len << std::endl;
    return 0;
}