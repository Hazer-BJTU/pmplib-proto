#include "ArithmeticFunctions.hpp"

using namespace rpc1k;

int64* get_data_pointer(const std::shared_ptr<BaseNum>& num) {
    //Dangerous operation! It is an undefined behavior.
    int64** ptr = reinterpret_cast<int64**>(num.get());
    return *(ptr + 1);
}

int main() {
    RealParser parser;
    auto num1 = std::make_shared<BaseNum>();
    auto num2 = std::make_shared<BaseNum>();
    auto num3 = std::make_shared<BaseNum>();
    auto num4 = std::make_shared<BaseNum>();
    parser(num1, "145945.113594679796E-2");
    parser(num2, "2579643.145962501001E-4");
    arithmetic_numerical_add_carry(get_data_pointer(num1), get_data_pointer(num2), get_data_pointer(num3));
    std::cout << parser(num3) << std::endl;
    std::cout << arithmetic_numerical_comp(get_data_pointer(num1), get_data_pointer(num2)) << std::endl;
    arithmetic_numerical_sub_carry(get_data_pointer(num1), get_data_pointer(num2), get_data_pointer(num3));
    std::cout << parser(num3) << std::endl;
    arithmetic_numerical_add_carry(get_data_pointer(num3), get_data_pointer(num2), get_data_pointer(num4));
    std::cout << arithmetic_numerical_comp(get_data_pointer(num1), get_data_pointer(num4)) << std::endl;
    arithmetic_numerical_multiply(get_data_pointer(num1), get_data_pointer(num2), get_data_pointer(num3), 0, LENGTH);
    arithmetic_numerical_carry(get_data_pointer(num3));
    std::cout << parser(num3) << std::endl;
    return 0;
}
