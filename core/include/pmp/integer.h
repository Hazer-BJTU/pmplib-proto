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
    IntegerDAGContext(const std::shared_ptr<Field>& field);
    friend Field;
    friend IntegerVarReference;
    friend void collect_graph_details(std::ostream& stream, const std::shared_ptr<IntegerDAGContext::Field>& field) noexcept;
    friend void collect_proce_details(std::ostream& stream, const std::shared_ptr<IntegerDAGContext::Field>& field) noexcept;
public:
    IntegerDAGContext(size_t precision, IOBasic iobasic);
    ~IntegerDAGContext();
    IntegerDAGContext(const IntegerDAGContext& context);
    IntegerDAGContext& operator = (const IntegerDAGContext& context);
    IntegerDAGContext(IntegerDAGContext&& context);
    IntegerDAGContext& operator = (IntegerDAGContext&& context);
    IntegerVarReference make_integer(const char* integer_str);
#ifdef MPENGINE_GRAPHV_DEBUG_OPTION
public:
#else
private:
#endif
    void export_graph_details(const char* dir_base_path);
    void nodes_sort();
    void generate_procedures();
};

class IntegerVarReference {
public:
    struct Field;
private:
    std::unique_ptr<Field> field;
    friend Field;
    friend IntegerDAGContext;
    friend void collect_graph_details(std::ostream& stream, const std::shared_ptr<IntegerDAGContext::Field>& field) noexcept;
    friend void collect_proce_details(std::ostream& stream, const std::shared_ptr<IntegerDAGContext::Field>& field) noexcept;
    friend std::ostream& operator << (std::ostream& stream, const IntegerVarReference& integer_ref) noexcept;
    friend IntegerVarReference operator + (IntegerVarReference& integer_A, IntegerVarReference& integer_B);
public:
    IntegerVarReference(const char* integer_str, IntegerDAGContext& context);
    IntegerVarReference(const char* integer_str, IntegerDAGContext&& context);
    ~IntegerVarReference();
    IntegerVarReference(const IntegerVarReference& integer_ref);
    IntegerVarReference& operator = (const IntegerVarReference& integer_ref);
    IntegerVarReference(IntegerVarReference&& integer_ref);
    IntegerVarReference& operator = (IntegerVarReference&& integer_ref);
    IntegerDAGContext get_context();
};

}

namespace pmp {

using io = mpengine::IOBasic;
using context = mpengine::IntegerDAGContext;
using integer = mpengine::IntegerVarReference;

}
