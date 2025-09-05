#pragma once

#include "Basics.h"
#include "ArithmeticFunctions.hpp"

namespace mpengine {

class ArithmeticAddNodeForInteger: public BasicBinaryOperation {
public:
    using NodeHandle = std::shared_ptr<BasicNodeType>;
private:
    struct ArithmeticAddTaskForInteger: public putils::Task {
        using DataRef = DataPtr&;
        DataRef source_A;
        DataRef source_B;
        DataRef target_C;
        ArithmeticAddTaskForInteger(DataRef source_A, DataRef source_B, DataRef target_C);
        ~ArithmeticAddTaskForInteger() override = default;
        void run() override;
        std::string description() const noexcept override;
        void allocate_data();
    };
public:
    ArithmeticAddNodeForInteger(NodeHandle& node_A, NodeHandle& node_B);
    ~ArithmeticAddNodeForInteger() override = default;
    void generate_procedure() override;
};

}
