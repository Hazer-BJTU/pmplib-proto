#pragma once

#include "Basics.h"
#include "ArithmeticFunctions.hpp"

namespace mpengine {

class ArithmeticAddNodeForInteger: public BasicBinaryOperation {
private:
    class ArithmeticAddTaskForInteger: public putils::Task {
    private:
        using DataArr = BasicIntegerType::ElementType*;
        DataArr source_A; bool& sign_A;
        DataArr source_B; bool& sign_B;
        DataArr target_C; bool& sign_C;
        size_t length;
        const BasicIntegerType::ElementType base;
    public: 
        ArithmeticAddTaskForInteger(
            DataArr source_A, bool& sign_A,
            DataArr source_B, bool& sign_B,
            DataArr target_C, bool& sign_C,
            size_t length,
            const BasicIntegerType::ElementType base
        );
        ~ArithmeticAddTaskForInteger() override = default;
        void run() override;
    };
public:
    ArithmeticAddNodeForInteger(BasicNodeType& node_A, BasicNodeType& node_B);
    ~ArithmeticAddNodeForInteger() override = default;
    void generate_procedure() override;
};

}
