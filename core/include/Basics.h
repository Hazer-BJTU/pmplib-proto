#pragma once

#include "FiniteStateMachine.hpp"
#include "MemoryAllocator.h"
#include "GlobalConfig.h"
#include "RuntimeLog.h"

namespace mpengine {

struct BasicIntegerType {
    using ElementType = uint64_t;
    using BlockHandle = putils::MemoryPool::BlockHandle;
    bool sign;
    size_t log_len, len;
    BlockHandle data;
    BasicIntegerType(size_t log_len);
    virtual ~BasicIntegerType();
    ElementType* get_pointer() const noexcept;
};

struct BasicNodeType {
    using DataPtr = std::shared_ptr<BasicIntegerType>;
    DataPtr data;
    BasicNodeType();
    virtual ~BasicNodeType();
};

struct BasicTransformation: public BasicNodeType {
    using NodePtr = BasicNodeType*;
    NodePtr operand;
    BasicTransformation();
    ~BasicTransformation() override;
};

struct BasicBinaryOperation: public BasicNodeType {
    using NodePtr = BasicNodeType*;
    NodePtr operand_A, operand_B;
    BasicBinaryOperation();
    ~BasicBinaryOperation() override;
};

struct ConstantNode: public BasicNodeType {
    bool referenced;
    ConstantNode(bool referenced = false);
    ConstantNode(const BasicNodeType& node, bool referenced = false);
    ~ConstantNode() override;
};

class IntegerParser: public Automaton {
private:
public:
};

}
