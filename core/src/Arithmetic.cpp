#include "Arithmetic.h"

namespace mpengine {

ArithmeticAddNodeForInteger::ArithmeticAddTaskForInteger::ArithmeticAddTaskForInteger(
    const DataHandle& source_A,
    const DataHandle& source_B,
    const DataHandle& target_C,
    const ComputeUnitPtr curr_unit
): source_A(source_A), 
   source_B(source_B), 
   target_C(target_C),
   curr_unit(curr_unit) { 
    if (curr_unit == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to bind a task to compute unit pointer (nullptr)!", "DAG construction error");
    }  
}

void ArithmeticAddNodeForInteger::ArithmeticAddTaskForInteger::run() {
    BasicIntegerType::ElementType* data_A = source_A->get_ensured_pointer();
    BasicIntegerType::ElementType* data_B = source_B->get_ensured_pointer();
    BasicIntegerType::ElementType* data_C = target_C->get_ensured_pointer();
    const size_t length = target_C->len;
    const BasicIntegerType::ElementType base = iofun::store_base(target_C->iobasic);
    bool flag = false;
    if (source_A->sign == source_B->sign) {
        //For integers with the same sign, simply add their absolute values directly.
        flag |= u64_variable_length_integer_addition_with_carry(data_A, data_B, data_C, length, base);
        target_C->sign = source_A->sign;
    } else {
        int comp_result = u64_variable_length_integer_compare(data_A, data_B, length);
        if (comp_result > 0) {
            // |A| > |B| : |C| = |A| - |B|
            flag |= u64_variable_length_integer_subtraction_with_carry_a_ge_b(data_A, data_B, data_C, length, base);
            if (source_A->sign == 0 && source_B->sign == 1) {
                // C = A + B = B - (-A) = |B| - |A| = -(|A| - |B|)
                target_C->sign = 0;
            } else if (source_A->sign == 1 && source_B->sign == 0) {
                // C = A + B = A - (-B) = |A| - |B|
                target_C->sign = 1;
            }
        } else if (comp_result < 0) {
            // |A| < |B| : |C| = |B| - |A|
            flag |= u64_variable_length_integer_subtraction_with_carry_a_ge_b(data_B, data_A, data_C, length, base);
            if (source_A->sign == 0 && source_B->sign == 1) {
                // C = A + B = B - (-A) = |B| - |A|
                target_C->sign = 1;
            } else if (source_A->sign == 1 && source_B->sign == 0) {
                // C = A + B = A - (-B) = |A| - |B| = -(|B| - |A|)
                target_C->sign = 0;
            }
        } else if (comp_result == 0) {
            target_C->sign = 1;
        }
    }
    if (flag) {
        putils::RuntimeLog::get_global_log().add("(Runtime computations): Unexpected integer calculation overflow occurred!", putils::RuntimeLog::Level::WARN);
    }
    source_A.reset();
    source_B.reset();
    target_C.reset();
    curr_unit->forward();
    return;
}

std::string ArithmeticAddNodeForInteger::ArithmeticAddTaskForInteger::description() const noexcept {
    std::stringstream ss;
    ss << "task[" << reinterpret_cast<uintptr_t>(this) << "]:arithmetic_add_integer:";
    ss << "sources[" << source_A->get_status() << "," << source_B->get_status() << "],target[" << target_C->get_status() << "]";
    return ss.str();
}

ArithmeticAddNodeForInteger::ArithmeticAddNodeForInteger(NodeHandle& node_A, NodeHandle& node_B) {
    node_A->nexts.emplace_back(this);
    node_B->nexts.emplace_back(this);
    operand_A = node_A.get();
    operand_B = node_B.get();
    if (operand_A->data == nullptr || operand_B->data == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Operands' datas are not initialized.", "DAG construction error");
    }
    if (operand_A->data->len != operand_B->data->len) {
        std::stringstream ss;
        ss << "Node data length mismatch: (" << operand_A->data->len << ") can not match (" << operand_B->data->len << ")!";
        throw PUTILS_GENERAL_EXCEPTION(ss.str(), "DAG construction error");
    }
    if (operand_A->data->iobasic != operand_B->data->iobasic) {
        std::stringstream ss;
        ss << "Node data iobasic mismatch: (" << iofun::base_name(operand_A->data->iobasic) << ") can not match (" << iofun::base_name(operand_B->data->iobasic) << ")!";
        throw PUTILS_GENERAL_EXCEPTION(ss.str(), "DAG construction error");
    }
    data = std::make_shared<BasicIntegerType>(operand_A->data->log_len, operand_A->data->iobasic);
}

void ArithmeticAddNodeForInteger::generate_procedure() {
    try {
        auto compute_unit_ptr = std::make_unique<MonoUnit<MultiTaskSynchronizer>>();
        compute_unit_ptr->add_task(std::make_shared<ArithmeticAddTaskForInteger>(operand_A->data, operand_B->data, data, compute_unit_ptr.get()));
        compute_unit_ptr->add_dependency(operand_A->get_procedure_port());
        compute_unit_ptr->add_dependency(operand_B->get_procedure_port());
        procedure.emplace_back(std::move(compute_unit_ptr));
    } PUTILS_CATCH_THROW_GENERAL
    return;
}

}
