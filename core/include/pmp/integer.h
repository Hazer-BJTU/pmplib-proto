#pragma once

#include <memory>
#include <iostream>

#include "IOBasic.hpp"

namespace mpengine {

class IntegerVarReference;

class IntegerDAGContext {
public:
    struct Field;
private:
    std::shared_ptr<Field> field;
    friend Field;
    friend IntegerVarReference;
public:
    IntegerDAGContext(size_t precision, IOBasic iobasic);
    ~IntegerDAGContext();
    IntegerDAGContext(const IntegerDAGContext& context);
    IntegerDAGContext& operator = (const IntegerDAGContext& context);
    IntegerDAGContext(IntegerDAGContext&& context);
    IntegerDAGContext& operator = (IntegerDAGContext&& context);
};

class IntegerVarReference {
public:
    struct Field;
private:
    std::unique_ptr<Field> field;
    friend Field;
    friend IntegerDAGContext;
    friend std::ostream& operator << (std::ostream& stream, const IntegerVarReference& integer_ref) noexcept;
public:
    IntegerVarReference(const char* integer_str, IntegerDAGContext& context);
    ~IntegerVarReference();
    IntegerVarReference(const IntegerVarReference& integer_ref);
    IntegerVarReference& operator = (const IntegerVarReference& integer_ref);
    IntegerVarReference(IntegerVarReference&& integer_ref);
    IntegerVarReference& operator = (IntegerVarReference&& integer_ref);
};

}

namespace pmp {

using io = mpengine::IOBasic;
using context = mpengine::IntegerDAGContext;
using integer = mpengine::IntegerVarReference;

}
