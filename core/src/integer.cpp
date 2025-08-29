#include "mpengine/integer.h"

#include <list>

#include "Basics.h"

namespace mpengine {

struct IntegerDAGContext::Field {
    using Signatures = std::list<IntegerVarReference*>;
    using NodeHandles = std::list<std::shared_ptr<BasicNodeType>>;
    Signatures signatures;
    NodeHandles nodes;
    size_t log_len;
};

struct IntegerVarReference::Field {
    using ContextPtr = std::shared_ptr<IntegerDAGContext>;
    using NodeHandle = std::shared_ptr<BasicNodeType>;
    using SignatureIt = IntegerDAGContext::Field::Signatures::iterator;
    ContextPtr context;
    NodeHandle node;
    SignatureIt signit;
};



}
