#include "pmp/integer.h"

#include <list>

#include "Basics.h"

namespace mpengine {

struct IntegerDAGContext::Field {
    using Signatures = std::list<IntegerVarReference*>;
    using NodeHandles = std::list<std::shared_ptr<BasicNodeType>>;
    Signatures signatures;
    NodeHandles nodes;
    size_t log_len;
    IOBasic iobasic;
};

struct IntegerVarReference::Field {
    using ContextPtr = std::shared_ptr<IntegerDAGContext::Field>;
    using NodeHandle = std::shared_ptr<BasicNodeType>;
    using SignatureIt = IntegerDAGContext::Field::Signatures::iterator;
    ContextPtr context;
    NodeHandle node;
    const SignatureIt signit;
};

IntegerDAGContext::IntegerDAGContext(size_t precesion, IOBasic iobasic) {
    field = std::make_shared<IntegerDAGContext::Field>(
        IntegerDAGContext::Field::Signatures(),
        IntegerDAGContext::Field::NodeHandles(),
        iofun::precision_to_log_len(precesion, iobasic),
        iobasic
    );
}

IntegerDAGContext::~IntegerDAGContext() {}

IntegerDAGContext::IntegerDAGContext(const IntegerDAGContext& context) {
    if (context.field == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to construct a new context using a released context object.", "context error");
    }
    field = context.field;
}

IntegerDAGContext& IntegerDAGContext::operator = (const IntegerDAGContext& context) {
    if (context.field == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to assign a released context to another context object.", "context error");
    }
    if (this == &context) {
        return *this;
    }
    field = context.field;
    return *this;
}

IntegerDAGContext::IntegerDAGContext(IntegerDAGContext&& context) {
    if (context.field == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to construct a new context using a released context object.", "context error");
    }
    field = context.field;
    context.field.reset();
}

IntegerDAGContext& IntegerDAGContext::operator = (IntegerDAGContext&& context) {
    if (context.field == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to assign a released context to another context object.", "context error");
    }
    if (this == &context) {
        return *this;
    }
    field = context.field;
    context.field.reset();
    return *this;
}

IntegerVarReference::IntegerVarReference(const char* integer_str, IntegerDAGContext& context) {
    std::string_view integer_view(integer_str);
    auto node = std::make_shared<ConstantNode>(context.field->log_len, context.field->iobasic);
    auto it = context.field->signatures.emplace(context.field->signatures.end(), this);
    context.field->nodes.emplace_back(node);
    field = std::make_unique<IntegerVarReference::Field>(context.field, node, it);
    try {
        parse_string_to_integer(integer_view, *(node->data));
    } PUTILS_CATCH_THROW_GENERAL
}

IntegerVarReference::~IntegerVarReference() {
    if (field) {
        field->context->signatures.erase(field->signit);
        field.reset();
    }
}

IntegerVarReference::IntegerVarReference(const IntegerVarReference& integer_ref) {
    if (integer_ref.field == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to construct a new integer using a released integer object.", "integer reference error");
    }
    auto it = integer_ref.field->context->signatures.emplace(
        integer_ref.field->context->signatures.end(),
        this
    );
    field = std::make_unique<IntegerVarReference::Field>(
        integer_ref.field->context, 
        integer_ref.field->node,
        it
    );
}

IntegerVarReference& IntegerVarReference::operator = (const IntegerVarReference& integer_ref) {
    if (integer_ref.field == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to assign a released integer to another integer object.", "integer reference error");
    }
    if (this == &integer_ref) {
        return *this;
    }
    auto it = integer_ref.field->context->signatures.emplace(
        integer_ref.field->context->signatures.end(),
        this
    );
    field = std::make_unique<IntegerVarReference::Field>(
        integer_ref.field->context, 
        integer_ref.field->node,
        it
    );
    return *this;
}

IntegerVarReference::IntegerVarReference(IntegerVarReference&& integer_ref) {
    if (integer_ref.field == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to construct a new integer using a released integer object.", "integer reference error");
    }
    auto it = integer_ref.field->context->signatures.emplace(
        integer_ref.field->context->signatures.end(),
        this
    );
    field = std::make_unique<IntegerVarReference::Field>(
        integer_ref.field->context, 
        integer_ref.field->node,
        it
    );
    integer_ref.field.reset();
}

IntegerVarReference& IntegerVarReference::operator = (IntegerVarReference&& integer_ref) {
    if (integer_ref.field == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to assign a released integer to another integer object.", "integer reference error");
    }
    if (this == &integer_ref) {
        return *this;
    }
    auto it = integer_ref.field->context->signatures.emplace(
        integer_ref.field->context->signatures.end(),
        this
    );
    field = std::make_unique<IntegerVarReference::Field>(
        integer_ref.field->context, 
        integer_ref.field->node,
        it
    );
    integer_ref.field.reset();
    return *this;
}

std::ostream& operator << (std::ostream& stream, const IntegerVarReference& integer_ref) noexcept {
    parse_integer_to_stream(stream, *(integer_ref.field->node->data));
    return stream;
}

}
