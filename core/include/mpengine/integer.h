#pragma once

#include <memory>

#include "IOBasic.h"

namespace mpengine {

class IntegerDAGContext {
private:
    struct Field;
    std::shared_ptr<Field> field;
    IntegerDAGContext(size_t precision, IOBasic iobasic);
};

class IntegerVarReference {
private:
    struct Field;
    std::unique_ptr<Field> field;
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
