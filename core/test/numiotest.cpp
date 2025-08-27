#include "Basics.h"

int main() {
    mpengine::BasicIntegerType X(10);
    std::ostringstream oss;
    mpengine::parse_string_to_integer("-0123409859123784987216349182730501293834928348", X);
    mpengine::parse_integer_to_stream(oss, X);
    std::cout << oss.str() << std::endl;
    return 0;
}