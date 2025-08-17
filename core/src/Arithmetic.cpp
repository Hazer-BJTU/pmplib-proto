#include "Arithmetic.h"

namespace mpengine {

ArithmeticAddNodeForInteger::ArithmeticAddTaskForInteger::ArithmeticAddTaskForInteger(
    DataArr source_A, bool& sign_A,
    DataArr source_B, bool& sign_B,
    DataArr target_C, bool& sign_C
    size_t length
): source_A(source_A), sign_A(sign_A),
   source_B(source_B), sign_B(sign_B),
   target_C(target_C), sign_C(sign_C),
   length(length) {}

void ArithmeticAddNodeForInteger::ArithmeticAddTaskForInteger::run() {
    if (sign_A == sign_B) {
        sign_C = sign_A;
        u64_variable_length_integer_addition_with_carry<BasicIntegerType::BASE>(
            source_A, source_B, target_C, length
        );
        return;
    } else if (sign_A == 0 && sign_B == 1) {
        
    }
}

}
