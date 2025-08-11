#include "Basics.h"

int main() {
    mpengine::BasicIntegerType X(10);
    mpengine::parse_string_to_integer("-0", X);
    std::cout << mpengine::parse_integer_to_string(X) << std::endl;
    return 0;
}