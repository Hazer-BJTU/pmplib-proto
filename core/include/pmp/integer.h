#pragma once

#include <memory>

#include "IOBasic.hpp"

namespace mpengine {

class IntegerDAGContext {
public:
    struct Field;
private:
    std::shared_ptr<Field> field;
public:
    IntegerDAGContext(size_t precision, IOBasic iobasic);
};

class IntegerVarReference {
public:
    struct Field;
private:
    std::unique_ptr<Field> field;
public:
    IntegerVarReference(IntegerDAGContext& context);
    ~IntegerVarReference();
    IntegerVarReference(const IntegerVarReference&);
    IntegerVarReference& operator = (const IntegerVarReference&);
    IntegerVarReference(IntegerVarReference&&);
    IntegerVarReference& operator = (IntegerVarReference&&);
};

}

namespace pmp {

using io = mpengine::IOBasic;
using context = mpengine::IntegerDAGContext;
using integer = mpengine::IntegerVarReference;

}
