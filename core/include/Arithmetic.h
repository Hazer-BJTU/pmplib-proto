#pragma once

#include "Basics.h"
#include "ArithmeticFunctions.hpp"

namespace mpengine {

class ArithmeticAddNodeForInteger: public BasicBinaryOperation {
public:
    using DataHandle = BasicNodeType::DataPtr;
    using NodeHandle = std::shared_ptr<BasicNodeType>;
    using ComputeUnitPtr = BasicComputeUnitType*;
private:
    struct ArithmeticAddTaskForInteger: public putils::Task {
        DataHandle source_A;
        DataHandle source_B;
        DataHandle target_C;
        const ComputeUnitPtr curr_unit;
        ArithmeticAddTaskForInteger(
            const DataHandle& source_A,
            const DataHandle& source_B,
            const DataHandle& target_C,
            const ComputeUnitPtr curr_unit
        );
        ~ArithmeticAddTaskForInteger() override = default;
        void run() override;
        std::string description() const noexcept override;
    };
public:
    ArithmeticAddNodeForInteger(NodeHandle& node_A, NodeHandle& node_B);
    ~ArithmeticAddNodeForInteger() override = default;
    void generate_procedure() override;
};

}
