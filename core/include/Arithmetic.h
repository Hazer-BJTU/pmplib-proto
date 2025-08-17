#pragma once

#include "Basics.h"
#include "ArithmeticFunctions.hpp"

namespace mpengine {

class ArithmeticAddNodeForInteger: public BasicBinaryOperation {
private:
    class ArithmeticAddTaskForInteger: public putils::Task {
    private:
        using DataArr = BasicIntegerType::ElementType*;
        DataArr source_A, source_B, target_C;
        bool &sign_A, &sign_B, &sign_C;
        size_t length;
    public: 
        ArithmeticAddTaskForInteger(
            DataArr source_A, bool& sign_A,
            DataArr source_B, bool& sign_B,
            DataArr target_C, bool& sign_C,
            size_t length
        );
        ~ArithmeticAddTaskForInteger() override = default;
        void run() override;
    };
public:
    
};

}
