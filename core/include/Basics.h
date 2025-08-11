#pragma once

#include <string.h>

#include "FiniteStateMachine.hpp"
#include "MemoryAllocator.h"
#include "GlobalConfig.h"
#include "RuntimeLog.h"

namespace mpengine {

struct BasicIntegerType {
    using ElementType = uint64_t;
    using BlockHandle = putils::MemoryPool::BlockHandle;
    static constexpr ElementType BASE = 100000000ull;
    static constexpr ElementType BB = 10ull;
    static constexpr size_t LOG_BASE = 8;
    bool sign;
    size_t log_len, len;
    BlockHandle data;
    BasicIntegerType(size_t log_len);
    virtual ~BasicIntegerType();
    ElementType* get_pointer() const noexcept;
};

struct BasicProcedureType {
    BasicProcedureType();
    virtual ~BasicProcedureType();
    BasicProcedureType(const BasicProcedureType&) = default;
    BasicProcedureType& operator = (const BasicProcedureType&) = default;
    BasicProcedureType(BasicProcedureType&&) = default;
    BasicProcedureType& operator = (BasicProcedureType&&) = default;
};

struct BasicNodeType {
    using DataPtr = std::shared_ptr<BasicIntegerType>;
    using ProcPtr = std::unique_ptr<BasicProcedureType>;
    using NodePtr = BasicNodeType*;
    DataPtr data;
    ProcPtr proc;
    NodePtr next;
    BasicNodeType();
    virtual ~BasicNodeType();
    BasicNodeType(const BasicNodeType&) = default;
    BasicNodeType& operator = (const BasicNodeType&) = default;
    BasicNodeType(BasicNodeType&&) = default;
    BasicNodeType& operator = (BasicNodeType&&) = default;
};

struct BasicTransformation: public BasicNodeType {
    NodePtr operand;
    BasicTransformation();
    ~BasicTransformation() override;
};

struct BasicBinaryOperation: public BasicNodeType {
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

void parse_string_to_integer(std::string_view integer_view, BasicIntegerType& data);
std::string parse_integer_to_string(const BasicIntegerType& data) noexcept;

}
