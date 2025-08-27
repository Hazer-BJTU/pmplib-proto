#include "mpengine/integer.h"

#include <bit>
#include <list>

#include "Basics.h"

namespace mpengine {

struct IntegerDAGContext {
    using Signatures = std::list<IntegerVarReference::VarSignature>;
    using NodeHandles = std::list<IntegerVarReference::NodeHandle>;
    Signatures signatures;
    NodeHandles nodes;
    IntegerDAGContext() = default;
    ~IntegerDAGContext() = default;
    IntegerDAGContext(const IntegerDAGContext&) = delete;
    IntegerDAGContext& operator = (const IntegerDAGContext&) = delete;
    IntegerDAGContext(IntegerDAGContext&&) = delete;
    IntegerDAGContext& operator = (IntegerDAGContext&&) = delete;
};

IntegerVarReference::IntegerVarReference(const NodeHandle& node): context(), node(node) {
    context->signatures.emplace_back(this);
    context->nodes.emplace_back(node);
}

IntegerVarReference::IntegerVarReference(size_t precision_digits, std::string_view str): 
IntegerVarReference(std::make_shared<ConstantNode>(precision_digits)) {
    try {
        parse_string_to_integer(str, *(node->data));
    } PUTILS_CATCH_THROW_GENERAL
}

}
