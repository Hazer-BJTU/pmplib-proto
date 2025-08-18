#include "Arithmetic.h"

namespace mpengine {

ArithmeticAddNodeForInteger::ArithmeticAddTaskForInteger::ArithmeticAddTaskForInteger(
    DataArr source_A, bool& sign_A,
    DataArr source_B, bool& sign_B,
    DataArr target_C, bool& sign_C,
    size_t length
): source_A(source_A), sign_A(sign_A),
   source_B(source_B), sign_B(sign_B),
   target_C(target_C), sign_C(sign_C),
   length(length) {}

void ArithmeticAddNodeForInteger::ArithmeticAddTaskForInteger::run() {
    bool flag = false;
    if (sign_A == sign_B) {
        flag |= u64_variable_length_integer_addition_with_carry<BasicIntegerType::BASE>(source_A, source_B, target_C, length);
        sign_C = sign_A;
    } else {
        int comp_result = u64_variable_length_integer_compare(source_A, source_B, length);
        if (comp_result > 0) {
            flag |= u64_variable_length_integer_subtraction_with_carry_a_ge_b<BasicIntegerType::BASE>(source_A, source_B, target_C, length);
            if (sign_A == 0 && sign_B == 1) {
                sign_C = 0;
            } else if (sign_A == 1 && sign_B == 0) {
                sign_C = 1;
            }
        } else if (comp_result < 0) {
            flag |= u64_variable_length_integer_subtraction_with_carry_a_ge_b<BasicIntegerType::BASE>(source_B, source_A, target_C, length);
            if (sign_A == 0 && sign_B == 1) {
                sign_C = 1;
            } else if (sign_A == 1 && sign_B == 0) {
                sign_C = 0;
            }
        } else if (comp_result == 0) {
            sign_C = 1;
        }
    }
    if (flag) {
        putils::RuntimeLog::get_global_log().add("(Runtime computations): Unexpected integer calculation overflow occurred!", putils::RuntimeLog::Level::WARN);
    }
    return;
}

ArithmeticAddNodeForInteger::ArithmeticAddNodeForInteger(BasicNodeType& node_A, BasicNodeType& node_B) {
    node_A.nexts.push_back(this);
    node_B.nexts.push_back(this);
    operand_A = &node_A;
    operand_B = &node_B;
}

void ArithmeticAddNodeForInteger::generate_procedure() {
    DataPtr data_A = operand_A->data, data_B = operand_B->data;
    if (data_A == nullptr || data_B == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Predecessor node data is not constructed.", "DAG construction error");
    }
    if (data_A->len != data_B->len) {
        std::stringstream ss;
        ss << "Node data length mismatch: (" << data_A->len << ") can not match (" << data_B->len << ")!";
        throw PUTILS_GENERAL_EXCEPTION(ss.str(), "DAG construction error");
    }
    data = std::make_shared<BasicIntegerType>(data_A->log_len);
    auto compute_unit_ptr = std::make_unique<MonoUnit<MultiTaskSynchronizer>>();
    compute_unit_ptr->task = std::make_shared<ArithmeticAddTaskForInteger>(
        data_A->get_pointer(), data_A->sign,
        data_B->get_pointer(), data_B->sign,
        data->get_pointer(), data->sign,
        data->len
    );
    compute_unit_ptr->add_dependency(operand_A->get_procedure_port());
    compute_unit_ptr->add_dependency(operand_B->get_procedure_port());
    procedure.emplace_back(std::move(compute_unit_ptr));
    return;
}

}
