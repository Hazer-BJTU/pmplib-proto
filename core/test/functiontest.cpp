#include "Basics.h"
#include "ArithmeticFunctions.hpp"

int main() {
    mpengine::BasicIntegerType X(10), Y(10), Z(10);
    mpengine::parse_string_to_integer("234999283409601923747912634980123423499928340960192374791263498012342349992834096019237479126349801234", X);
    mpengine::parse_string_to_integer("199902930400509129394756712839405819990293040050912939475671283940581999029304005091293947567128394058", Y);
    mpengine::u64_variable_length_integer_multiplication_c_2len_with_carry<mpengine::BasicIntegerType::BASE>(X.get_pointer(), Y.get_pointer(), Z.get_pointer(), X.len);
    std::ostringstream oss;
    mpengine::parse_integer_to_stream(oss, Z);
    std::cout << oss.str() << std::endl;
    return 0;
}