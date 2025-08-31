#include "pmp/integer.h"

#include <set>
#include <list>
#include <fstream>

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
    stream << "{" << std::endl;
    //Start of node groups.
    stream << "\t" << "\"nodes_groups\": [" << std::endl;
    //Node group #1, integer references.
    stream << "\t\t" << "{" << std::endl;
    stream << "\t\t\t" << "\"node_list\": [" << std::endl;
    size_t idx = 0;
    for (auto it = field->signatures.begin(); it != field->signatures.end(); it++) {
        idx++;
        stream << "\t\t\t\t" << "{" << std::endl;
        stream << "\t\t\t\t\t" << "\"index\": " << reinterpret_cast<uintptr_t>(*it) << "," << std::endl;
        stream << "\t\t\t\t\t" << "\"label\": " << "\"reference#" << idx << "\"" << std::endl;
        stream << "\t\t\t\t" << "}";
        if (std::next(it) != field->signatures.end()) {
            stream << "," << std::endl;
        } else {
            stream << std::endl;
        }
    }
    stream << "\t\t\t" << "]," << std::endl;
    stream << "\t\t\t" << "\"display_configs\": {" << std::endl;
    stream << "\t\t\t\t" << "\"node_color\": \"red\"" << std::endl;
    stream << "\t\t\t" << "}," << std::endl;
    stream << "\t\t\t" << "\"label_configs\": {" << std::endl;
    stream << "\t\t\t\t" << "\"font_size\": 12" << std::endl;
    stream << "\t\t\t" << "}" << std::endl;
    stream << "\t\t" << "}," << std::endl;
    //Node group #2, compute nodes.
    stream << "\t\t" << "{" << std::endl;
    stream << "\t\t\t" << "\"node_list\": [" << std::endl;
    idx = 0;
    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
        idx++;
        stream << "\t\t\t\t" << "{" << std::endl;
        stream << "\t\t\t\t\t" << "\"index\": " << reinterpret_cast<uintptr_t>(it->get()) << "," << std::endl;
        stream << "\t\t\t\t\t" << "\"label\": " << "\"dag_node#" << idx << "\"" << std::endl;
        stream << "\t\t\t\t" << "}";
        if (std::next(it) != field->nodes.end()) {
            stream << "," << std::endl;
        } else {
            stream << std::endl;
        }
    }
    stream << "\t\t\t" << "]," << std::endl;
    stream << "\t\t\t" << "\"display_configs\": {" << std::endl;
    stream << "\t\t\t\t" << "\"node_color\": \"blue\"" << std::endl;
    stream << "\t\t\t" << "}," << std::endl;
    stream << "\t\t\t" << "\"label_configs\": {" << std::endl;
    stream << "\t\t\t\t" << "\"font_size\": 12" << std::endl;
    stream << "\t\t\t" << "}" << std::endl;
    stream << "\t\t" << "}," << std::endl;
    //Node group #3, data domain.
    stream << "\t\t" << "{" << std::endl;
    stream << "\t\t\t" << "\"node_list\": [" << std::endl;
    idx = 0;
    std::set<uintptr_t> data_index;
    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
        data_index.insert(reinterpret_cast<uintptr_t>((*it)->data.get()));
    }
    for (auto it = data_index.begin(); it != data_index.end(); it++) {
        idx++;
        stream << "\t\t\t\t" << "{" << std::endl;
        stream << "\t\t\t\t\t" << "\"index\": " << *it << ", " << std::endl;
        stream << "\t\t\t\t\t" << "\"label\": " << "\"data#" << idx << "\"" << std::endl;
        stream << "\t\t\t\t" << "}";
        if (std::next(it) != data_index.end()) {
            stream << "," << std::endl;
        } else {
            stream << std::endl;
        }
    }
    stream << "\t\t\t" << "]," << std::endl;
    stream << "\t\t\t" << "\"display_configs\": {" << std::endl;
    stream << "\t\t\t\t" << "\"node_color\": \"green\"" << std::endl;
    stream << "\t\t\t" << "}," << std::endl;
    stream << "\t\t\t" << "\"label_configs\": {" << std::endl;
    stream << "\t\t\t\t" << "\"font_size\": 12" << std::endl;
    stream << "\t\t\t" << "}" << std::endl;
    stream << "\t\t" << "}" << std::endl;
    //End of node groups.
    stream << "\t" << "]," << std::endl;
    //Start of edge groups.
    stream << "\t" << "\"edges_groups\": [" << std::endl;
    //Edge group #1, reference -> node.
    stream << "\t\t" << "{" << std::endl;
    stream << "\t\t\t" << "\"edge_list\": [" << std::endl;
    for (auto it = field->signatures.begin(); it != field->signatures.end(); it++) {
        stream << "\t\t\t\t" << "{" << std::endl;
        stream << "\t\t\t\t\t" << "\"source\": " << reinterpret_cast<uintptr_t>(*it) << "," << std::endl;
        stream << "\t\t\t\t\t" << "\"target\": " << reinterpret_cast<uintptr_t>((*it)->field->node.get()) << std::endl;
        stream << "\t\t\t\t" << "}";
        if (std::next(it) != field->signatures.end()) {
            stream << "," << std::endl;
        } else {
            stream << std::endl;
        }
    }
    stream << "\t\t\t" << "]" << std::endl;
    stream << "\t\t" << "}," << std::endl;
    //Edge group #2, node -> data.
    stream << "\t\t" << "{" << std::endl;
    stream << "\t\t\t" << "\"edge_list\": [" << std::endl;
    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
        stream << "\t\t\t\t" << "{" << std::endl;
        stream << "\t\t\t\t\t" << "\"source\": " << reinterpret_cast<uintptr_t>((*it).get()) << "," << std::endl;
        stream << "\t\t\t\t\t" << "\"target\": " << reinterpret_cast<uintptr_t>((*it)->data.get()) << std::endl;
        stream << "\t\t\t\t" << "}";
        if (std::next(it) != field->nodes.end()) {
            stream << "," << std::endl;
        } else {
            stream << std::endl;
        }
    }
    stream << "\t\t\t" << "]" << std::endl;
    stream << "\t\t" << "}" << std::endl;
    //End of edge groups.
    stream << "\t" << "]" << std::endl;
    stream << "}" << std::endl;
    return;
}

}
