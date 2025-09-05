#include "pmp/integer.h"

#include <set>
#include <list>
#include <fstream>

#include "Basics.h"
#include "StructuredNotation.hpp"

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

IntegerDAGContext::IntegerDAGContext(const std::shared_ptr<Field>& field): field(field) {}

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

IntegerVarReference IntegerDAGContext::make_integer(const char* integer_str) {
    try {
        return IntegerVarReference(integer_str, *this);
    } PUTILS_CATCH_THROW_GENERAL
}

void export_context_details(std::ostream& stream, std::shared_ptr<IntegerDAGContext::Field>& field) noexcept;

#ifdef MPENGINE_GRAPHV_DEBUG_OPTION
void IntegerDAGContext::export_graph_details(const char* file_path) {
    try {
        std::ofstream file_out(file_path);
        if (!file_out.is_open()) {
            std::stringstream ss;
            ss << "Failed to export graph details to: " << file_path << "!";
            throw PUTILS_GENERAL_EXCEPTION(ss.str(), "I/O error");
        }
        if (field == nullptr) {
            throw PUTILS_GENERAL_EXCEPTION("Unable to export details of a released context object.", "context error");
        }
        export_context_details(file_out, field);
        return;
    } PUTILS_CATCH_THROW_GENERAL
}
#endif

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

IntegerVarReference::IntegerVarReference(const char* integer_str, IntegerDAGContext&& context) {
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
    integer_ref.~IntegerVarReference();
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
    integer_ref.~IntegerVarReference();
    return *this;
}

IntegerDAGContext IntegerVarReference::get_context() {
    if (field == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to acquire context of a released integer object.", "integer reference error");
    }
    return IntegerDAGContext(field->context);
}

std::ostream& operator << (std::ostream& stream, const IntegerVarReference& integer_ref) noexcept {
    parse_integer_to_stream(stream, *(integer_ref.field->node->data));
    return stream;
}

void export_context_details(std::ostream& stream, std::shared_ptr<IntegerDAGContext::Field>& field) noexcept {
    stn::beg_notation();
        stn::beg_list("nodes_groups");
            stn::beg_field();
                stn::beg_list("node_list");
                    size_t idx = 0;
                    for (auto it = field->signatures.begin(); it != field->signatures.end(); it++) {
                        idx++;
                        stn::beg_field();
                            stn::entry("index", reinterpret_cast<uintptr_t>(*it));
                            stn::entry("label", (std::ostringstream() << "reference#" << idx).str());
                        stn::end_field();
                    }
                stn::end_list();
                stn::beg_field("display_configs");
                    stn::entry("node_color", "red");
                    stn::entry("alpha", 0.3);
                stn::end_field();
                stn::beg_field("label_configs");
                    stn::entry("font_size", 5);
                    stn::entry("font_family", "monospace");
                stn::end_field();
            stn::end_field();
            stn::beg_field();
                stn::beg_list("node_list");
                    idx = 0;
                    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
                        idx++;
                        stn::beg_field();
                            stn::entry("index", reinterpret_cast<uintptr_t>(it->get()));
                            stn::entry("label", (std::ostringstream() << "dag_node#" << idx).str());
                        stn::end_field();
                    }
                stn::end_list();
                stn::beg_field("display_configs");
                    stn::entry("node_color", "blue");
                    stn::entry("alpha", 0.3);
                stn::end_field();
                stn::beg_field("label_configs");
                    stn::entry("font_size", 5);
                    stn::entry("font_family", "monospace");
                stn::end_field();
            stn::end_field();
            stn::beg_field();
                stn::beg_list("node_list");
                    idx = 0;
                    std::set<uintptr_t> data_index;
                    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
                        data_index.insert(reinterpret_cast<uintptr_t>((*it)->data.get()));
                    }
                    for (auto it = data_index.begin(); it != data_index.end(); it++) {
                        idx++;
                        stn::beg_field();
                            stn::entry("index", *it);
                            stn::entry("label", (std::ostringstream() << "data#" << idx).str());
                        stn::end_field();
                    }
                stn::end_list();
                stn::beg_field("display_configs");
                    stn::entry("node_color", "green");
                    stn::entry("alpha", 0.3);
                stn::end_field();
                stn::beg_field("label_configs");
                    stn::entry("font_size", 5);
                    stn::entry("font_family", "monospace");
                stn::end_field();
            stn::end_field();
        stn::end_list();
        stn::beg_list("edges_groups");
            stn::beg_field();
                stn::beg_list("edge_list");
                    for (auto it = field->signatures.begin(); it != field->signatures.end(); it++) {
                        stn::beg_field();
                            stn::entry("source", reinterpret_cast<uintptr_t>(*it));
                            stn::entry("target", reinterpret_cast<uintptr_t>((*it)->field->node.get()));
                        stn::end_field();
                    }
                stn::end_list();
                stn::beg_field("display_configs");
                    stn::entry("width", 1.5);
                    stn::entry("edge_color", "gray");
                stn::end_field();
            stn::end_field();
            stn::beg_field();
                stn::beg_list("edge_list");
                    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
                        stn::beg_field();
                            stn::entry("source", reinterpret_cast<uintptr_t>((*it).get()));
                            stn::entry("target", reinterpret_cast<uintptr_t>((*it)->data.get()));
                        stn::end_field();
                    }
                stn::end_list();
                stn::beg_field("display_configs");
                    stn::entry("width", 1.5);
                    stn::entry("edge_color", "gray");
                stn::end_field();
            stn::end_field();
        stn::end_list();
    stn::end_notation(stream);
    return;
}

}
