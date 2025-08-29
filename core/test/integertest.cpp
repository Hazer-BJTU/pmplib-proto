#include <iostream>
#include "pmp/integer.h"
#include "IOFunctions.h"

int main() {
    size_t log_len = mpengine::iofun::precision_to_log_len(1000, pmp::io::hex);
    std::cout << log_len << std::endl;
    return 0;
}