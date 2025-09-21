#include "pmp/integer.h"

#include <set>
#include <list>
#include <fstream>
#include <filesystem>

#include "Basics.h"
#include "Arithmetic.h"
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

bool nodes_topological_sort(IntegerDAGContext::Field::NodeHandles& node_handle_list) noexcept;

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

void IntegerDAGContext::export_graph_details(const char* dir_base_path) {
    if (field == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to export details of a released context object.", "context error");
    }
    try {
        std::filesystem::path dir_path(dir_base_path);
        dir_path.append("daginfo");
        bool new_dir_created = std::filesystem::create_directories(dir_path);
        std::filesystem::path file_path = dir_path / "dag.json";
        std::ofstream file_out(file_path);
        if (!file_out.is_open()) {
            std::stringstream ss;
            ss << "Failed to export graph details to: " << file_path << "!";
            throw PUTILS_GENERAL_EXCEPTION(ss.str(), "I/O error");
        }
        collect_graph_details(file_out, field);
        file_out.close();
        file_path = dir_path / "pro.json";
        file_out.open(file_path);
        if (!file_out.is_open()) {
            std::stringstream ss;
            ss << "Failed to export graph details to: " << file_path << "!";
            throw PUTILS_GENERAL_EXCEPTION(ss.str(), "I/O error");
        }
        collect_proce_details(file_out, field);
        file_out.close();
        return;
    } PUTILS_CATCH_THROW_GENERAL
}

void IntegerDAGContext::nodes_sort() {
    if (field->nodes.size() <= 1) {
        return;
    }
    bool flag = nodes_topological_sort(field->nodes);
    if (!flag) {
        throw PUTILS_GENERAL_EXCEPTION("Loop detected in a DAG!", "context error");
    }
    return;
}

void IntegerDAGContext::generate_procedures() {
    for (auto& node_handle: field->nodes) {
        try {
            node_handle->generate_procedure();
        } PUTILS_CATCH_THROW_GENERAL
    }
    return;
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
    field->node = integer_ref.field->node;
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
    field->node = integer_ref.field->node;
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

void collect_graph_details(std::ostream& stream, const std::shared_ptr<IntegerDAGContext::Field>& field) noexcept {
    stn::beg_notation();
        stn::beg_field("nodes_groups");
            stn::beg_field("references");
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
            stn::beg_field("dag_nodes");
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
            stn::beg_field("datas");
                stn::beg_list("node_list");
                    idx = 0;
                    std::set<uintptr_t> data_index;
                    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
                        if ((*it)->data != nullptr) {
                            data_index.insert(reinterpret_cast<uintptr_t>((*it)->data.get()));
                        }
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
            stn::beg_field("procedure");
                stn::beg_list("node_list");
                    idx = 0;
                    std::set<uintptr_t> unit_index;
                    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
                        for (auto jt = (*it)->procedure.begin(); jt != (*it)->procedure.end(); jt++) {
                            unit_index.insert(reinterpret_cast<uintptr_t>(jt->get()));
                        }
                    }
                    for (auto it = unit_index.begin(); it != unit_index.end(); it++) {
                        idx++;
                        stn::beg_field();
                            stn::entry("index", *it);
                            stn::entry("label", (std::ostringstream() << "unit#" << idx).str());
                        stn::end_field();
                    }
                stn::end_list();
                stn::beg_field("display_configs");
                    stn::entry("node_color", "purple");
                    stn::entry("alpha", 0.3);
                stn::end_field();
                stn::beg_field("label_configs");
                    stn::entry("font_size", 5);
                    stn::entry("font_family", "monospace");
                stn::end_field();
            stn::end_field();
        stn::end_field();
        stn::beg_field("edges_groups");
            stn::beg_field("references_nodes");
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
            stn::beg_field("nodes_datas");
                stn::beg_list("edge_list");
                    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
                        if ((*it)->data != nullptr) {
                            stn::beg_field();
                                stn::entry("source", reinterpret_cast<uintptr_t>((*it).get()));
                                stn::entry("target", reinterpret_cast<uintptr_t>((*it)->data.get()));
                            stn::end_field();
                        }
                    }
                stn::end_list();
                stn::beg_field("display_configs");
                    stn::entry("width", 1.5);
                    stn::entry("edge_color", "gray");
                stn::end_field();
            stn::end_field();
            stn::beg_field("nodes_nodes");
                stn::beg_list("edge_list");
                    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
                        auto& node_list = (*it)->nexts;
                        for (auto next_it = node_list.begin(); next_it != node_list.end(); next_it++) {
                            stn::beg_field();
                                stn::entry("source", reinterpret_cast<uintptr_t>((*it).get()));
                                stn::entry("target", reinterpret_cast<uintptr_t>(*next_it));
                            stn::end_field();
                        }
                    }
                stn::end_list();
                stn::beg_field("display_configs");
                    stn::entry("width", 1.5);
                    stn::entry("edge_color", "blue");
                stn::end_field();
            stn::end_field();
            stn::beg_field("units_units");
                stn::beg_list("edge_list");
                    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
                        for (auto jt = (*it)->procedure.begin(); jt != (*it)->procedure.end(); jt++) {
                            auto kt = std::next(jt);
                            if (kt != (*it)->procedure.end()) {
                                stn::beg_field();
                                    stn::entry("source", reinterpret_cast<uintptr_t>(jt->get()));
                                    stn::entry("target", reinterpret_cast<uintptr_t>(kt->get()));
                                stn::end_field();
                            }
                        }
                    }
                stn::end_list();
                stn::beg_field("display_configs");
                    stn::entry("width", 1.5);
                    stn::entry("edge_color", "purple");
                stn::end_field();
            stn::end_field();
            stn::beg_field("nodes_procedures");
                stn::beg_list("edge_list");
                    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
                        if ((*it)->procedure.size() != 0) {
                            stn::beg_field();
                                stn::entry("source", reinterpret_cast<uintptr_t>((*it).get()));
                                stn::entry("target", reinterpret_cast<uintptr_t>((*it)->procedure.front().get()));
                            stn::end_field();
                        }
                    }
                stn::end_list();
                stn::beg_field("display_configs");
                    stn::entry("width", 1.5);
                    stn::entry("edge_color", "purple");
                stn::end_field();
            stn::end_field();
        stn::end_field();
    stn::end_notation(stream);
    return;
}

void collect_proce_details(std::ostream& stream, const std::shared_ptr<IntegerDAGContext::Field>& field) noexcept {
    std::set<uintptr_t> unit_index;
    for (auto it = field->nodes.begin(); it != field->nodes.end(); it++) {
        for (auto jt = (*it)->procedure.begin(); jt != (*it)->procedure.end(); jt++) {
            unit_index.insert(reinterpret_cast<uintptr_t>(jt->get()));
        }
    }
    size_t idx = 0;
    stn::beg_notation();
        stn::beg_list("compute_units");
            for (uintptr_t intptr: unit_index) {
                idx++;
                auto unit_ptr = reinterpret_cast<BasicComputeUnitType*>(intptr);
                stn::beg_field();
                    stn::entry("name", (std::ostringstream() << "unit#" << idx).str());
                    stn::entry("index", intptr);
                    stn::entry("type", unit_ptr->get_type());
                    stn::entry("dependency_type", unit_ptr->get_acceptance());
                    if (unit_ptr->forward_calls.size() == 0) {
                        stn::entry("forward_signal", "NO_FORWARDS");
                    } else if (unit_ptr->forward_calls.size() == 1) {
                        stn::entry("forward_signal", "SERIALIZE_SIGNAL");
                    } else {
                        stn::entry("forward_signal", "DEFAULT_SIGNAL");
                    }
                    unit_ptr->generate_task_stn();
                    #ifdef MPENGINE_STORE_PROCEDURE_DETAILS
                        if (unit_ptr->forward_detas.size() != 0) {
                            stn::beg_list("forward_details");
                                for (auto it = unit_ptr->forward_detas.begin(); it != unit_ptr->forward_detas.end(); it++) {
                                    stn::entry(*it);
                                }
                            stn::end_list();
                        } else {
                            stn::entry("forward_details", "empty");
                        }
                    #else
                        stn::entry("forward_details_disabled", true);
                    #endif
                stn::end_field();
            }
        stn::end_list();
    stn::end_notation(stream);
    return;
}

bool nodes_topological_sort(IntegerDAGContext::Field::NodeHandles& node_handle_list) noexcept {
    using NodeHandle = std::shared_ptr<BasicNodeType>;
    using NodeHandles = IntegerDAGContext::Field::NodeHandles;
    using NodePtr = BasicNodeType*;
    NodeHandles sorted_node_handle_list;
    std::deque<NodePtr> node_queue;
    std::unordered_map<NodePtr, std::pair<NodeHandles::iterator, size_t>> node_degs;
    for (auto it = node_handle_list.begin(); it != node_handle_list.end(); it++) {
        node_degs[it->get()] = std::make_pair(it, 0);
    }
    for (const auto& node_handle: node_handle_list) {
        for (const auto next_node_ptr: node_handle->nexts) {
            node_degs[next_node_ptr].second++;
        }
    }
    for (const auto& [node_ptr, node_info]: node_degs) {
        if (node_info.second == 0) {
            node_queue.push_back(node_ptr);
        }
    }
    while(!node_queue.empty()) {
        auto node_ptr = node_queue.front();
        node_queue.pop_front();
        sorted_node_handle_list.emplace_back(*(node_degs[node_ptr].first));
        for (const auto next_node_ptr: node_ptr->nexts) {
            auto& node_info = node_degs[next_node_ptr];
            if (--node_info.second == 0) {
                node_queue.push_back(node_info.first->get());
            }
        }
    }
    const bool ret_flag = node_handle_list.size() == sorted_node_handle_list.size();
    node_handle_list = std::move(sorted_node_handle_list);
    return ret_flag;
}

IntegerVarReference operator + (IntegerVarReference& integer_A, IntegerVarReference& integer_B) {
    if (integer_A.field->context != integer_B.field->context) {
        throw PUTILS_GENERAL_EXCEPTION("Unable to add two integers of different contexts!", "arithmetic error");
    }
    auto& context_ptr = integer_A.field->context;
    IntegerVarReference integer_result = integer_A;
    integer_result.field->node = std::make_shared<ArithmeticAddNodeForInteger>(
        integer_A.field->node, integer_B.field->node
    );
    context_ptr->nodes.emplace_back(integer_result.field->node);
    return integer_result;
}

}
