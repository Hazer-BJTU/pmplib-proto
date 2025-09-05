#include "Arithmetic.h"

namespace mpengine {

void ArithmeticAddNodeForInteger::ArithmeticAddTaskForInteger::allocate_data() {
    if (target_C != nullptr) {
        return;
    }
    if (source_A == nullptr || source_B == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Predecessor node data is not constructed.", "DAG construction error");
    }
    if (source_A->len != source_B->len) {
        std::stringstream ss;
        ss << "Node data length mismatch: (" << source_A->len << ") can not match (" << source_B->len << ")!";
        throw PUTILS_GENERAL_EXCEPTION(ss.str(), "DAG construction error");
    }
    if (source_A->iobasic != source_B->iobasic) {
        std::stringstream ss;
        ss << "Node data iobasic mismatch: (" << iofun::base_name(source_A->iobasic) << ") can not match (" << iofun::base_name(source_B->iobasic) << ")!";
        throw PUTILS_GENERAL_EXCEPTION(ss.str(), "DAG construction error");
    }
    target_C = std::make_shared<BasicIntegerType>(source_A->log_len, source_A->iobasic);
    return;
}

ArithmeticAddNodeForInteger::ArithmeticAddTaskForInteger::ArithmeticAddTaskForInteger(
    DataRef source_A, DataRef source_B, DataRef target_C
): source_A(source_A), source_B(source_B), target_C(target_C) {
    static const bool delayed_allocation = mpengine::GlobalConfig::get_global_config().get_or_else(
        "Configurations/core/MemoryPreference/delayed_allocation", true
    );
    if (!delayed_allocation) {
        try {
            allocate_data();
        } PUTILS_CATCH_THROW_GENERAL
    }
}

void ArithmeticAddNodeForInteger::ArithmeticAddTaskForInteger::run() {
    try {
        allocate_data();
    } PUTILS_CATCH_THROW_GENERAL
    BasicIntegerType::ElementType* data_A = source_A->get_pointer();
    BasicIntegerType::ElementType* data_B = source_B->get_pointer();
    BasicIntegerType::ElementType* data_C = target_C->get_pointer();
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
    return;
}

std::string ArithmeticAddNodeForInteger::ArithmeticAddTaskForInteger::description() const noexcept {
    std::stringstream ss;
    ss << "task[" << reinterpret_cast<uintptr_t>(this) << "]:arithmetic_add_integer:";
    ss << "sources[" << reinterpret_cast<uintptr_t>(source_A.get()) << "," << reinterpret_cast<uintptr_t>(source_B.get()) << "],";
    if (target_C == nullptr) {
        ss << "target[null_yet]";
    } else {
        ss << "target[" << reinterpret_cast<uintptr_t>(target_C.get()) << "]";
    }
    return ss.str();
}

ArithmeticAddNodeForInteger::ArithmeticAddNodeForInteger(NodeHandle& node_A, NodeHandle& node_B) {
    node_A->nexts.emplace_back(this);
    node_B->nexts.emplace_back(this);
    operand_A = node_A.get();
    operand_B = node_B.get();
}

void ArithmeticAddNodeForInteger::generate_procedure() {
    try {
        auto compute_unit_ptr = std::make_unique<MonoUnit<MultiTaskSynchronizer>>();
        compute_unit_ptr->task = std::make_shared<ArithmeticAddTaskForInteger>(operand_A->data, operand_B->data, data);
        compute_unit_ptr->add_dependency(operand_A->get_procedure_port());
        compute_unit_ptr->add_dependency(operand_B->get_procedure_port());
        procedure.emplace_back(std::move(compute_unit_ptr));
        return;
    } PUTILS_CATCH_THROW_GENERAL
}

}
