#pragma once

#include <memory>
#include <string_view>

namespace mpengine {

struct IntegerDAGContext;
struct BasicNodeType;

class IntegerVarReference {
public:
    using VarSignature = IntegerVarReference*;
    using ContextPtr = std::shared_ptr<IntegerDAGContext>;
    using NodeHandle = std::shared_ptr<BasicNodeType>;
private:
    ContextPtr context;
    NodeHandle node;
public:
    IntegerVarReference(const NodeHandle& node);
    IntegerVarReference(size_t precision_digis, std::string_view str);
    ~IntegerVarReference();
    IntegerVarReference(const IntegerVarReference&);
    IntegerVarReference& operator = (const IntegerVarReference&);
    IntegerVarReference(IntegerVarReference&&);
    IntegerVarReference& operator = (IntegerVarReference&&);
};

}

namespace pmp {

using integer = mpengine::IntegerVarReference;

}
