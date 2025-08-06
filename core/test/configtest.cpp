#include "GlobalConfig.h"

int main() {
    using namespace mpengine;
    std::cout << cs::except(cs::any, cs::concate(cs::whitespace, cs::control));
    return 0;
}