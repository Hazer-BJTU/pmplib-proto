#include "Basics.h"

namespace mpengine {

BasicIntegerType::BasicIntegerType(size_t log_len, IOBasic iobasic): 
sign(1), log_len(log_len), len(0), data(nullptr), iobasic(iobasic) {
    //auto& memorypool = putils::MemoryPool::get_global_memorypool();
    static const size_t min_log_length = GlobalConfig::get_global_config().get_or_else<int64_t>(
        "Configurations/core/BasicIntegerType/limits/min_log_length", 0ll
    );
    static const size_t max_log_length = GlobalConfig::get_global_config().get_or_else<int64_t>(
        "Configurations/core/BasicIntegerType/limits/max_log_length", 0ll
    );
    static const bool delayed_allocation = GlobalConfig::get_global_config().get_or_else<bool>(
        "Configurations/core/MemoryPreference/delayed_allocation", true
    );
    if (max_log_length <= min_log_length) {
        throw PUTILS_GENERAL_EXCEPTION("Failed to fetch configurations.", "basic integer init error");
    }
    if (log_len < min_log_length) {
        log_len = min_log_length;
        putils::RuntimeLog::get_global_log().add(
            "(Basic Integer): The data length is implicitly truncated to the lower bound.", putils::RuntimeLog::Level::INFO
        );
    }
    if (log_len > max_log_length) {
        log_len = max_log_length;
        putils::RuntimeLog::get_global_log().add(
            "(Basic Integer): The data length is implicitly truncated to the upper bound.", putils::RuntimeLog::Level::INFO
        );
    }
    len = 1ull << log_len;
    if (!delayed_allocation) {
        try {
            allocate();
        } PUTILS_CATCH_THROW_GENERAL
    }
}

BasicIntegerType::~BasicIntegerType() {
    putils::release(data);
}

void BasicIntegerType::allocate() {
    if (data != nullptr) {
        return;
    }
    try {
        data = putils::MemoryPool::get_global_memorypool().allocate(len * sizeof(ElementType));
        memset(data->get<ElementType>(), 0, len * sizeof(ElementType));
    } PUTILS_CATCH_THROW_GENERAL
    return;
}

BasicIntegerType::ElementType* BasicIntegerType::get_pointer() const noexcept {
    return data->get<ElementType>();
}

BasicIntegerType::ElementType* BasicIntegerType::get_ensured_pointer() {
    try {
        allocate();
    } PUTILS_CATCH_THROW_GENERAL
    return data->get<ElementType>();
}

const char* BasicIntegerType::get_status() const noexcept {
    if (data == nullptr) {
        return "null_yet";
    } else {
        return "allocated";
    }
}

BasicComputeUnitType::BasicComputeUnitType(): forward_calls() {}

BasicComputeUnitType::~BasicComputeUnitType() {}

void BasicComputeUnitType::dependency_notice(int signal) {}

void BasicComputeUnitType::forward() {}

void BasicComputeUnitType::add_dependency(BasicComputeUnitType& predecessor) {}

void BasicComputeUnitType::add_task(const TaskPtr& task_ptr) {}

const char* BasicComputeUnitType::get_acceptance() const noexcept {
    return "[Starting unit, no predecessor]";
}

const char* BasicComputeUnitType::get_type() const noexcept {
    return "(Basic)";
}

void BasicComputeUnitType::generate_task_stn() const noexcept {
    stn::entry("task_descriptions", "empty");
    return;
}

BasicNodeType::BasicNodeType(): data(nullptr), nexts(), procedure() {}

BasicNodeType::~BasicNodeType() {}

void BasicNodeType::generate_procedure() {}

BasicComputeUnitType& BasicNodeType::get_procedure_port() {
    if (procedure.empty()) {
        throw PUTILS_GENERAL_EXCEPTION("Node procedure is not initialized.", "DAG construction error");
    }
    return *(procedure.back());
}

BasicTransformation::BasicTransformation(): operand(nullptr) {}

BasicTransformation::~BasicTransformation() {}

BasicBinaryOperation::BasicBinaryOperation(): operand_A(nullptr), operand_B(nullptr) {}

BasicBinaryOperation::~BasicBinaryOperation() {}

ConstantNode::ConstantNode(size_t log_len, IOBasic iobasic) {
    data = std::make_shared<BasicIntegerType>(log_len, iobasic);
}

ConstantNode::ConstantNode(const BasicNodeType& node) {
    data = node.data;
    if (data == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Attempt to construct constant node using node with empty data domain.", "DAG construction error");
    }
}

ConstantNode::~ConstantNode() {}

void ConstantNode::generate_procedure() {
    if (data == nullptr) {
        throw PUTILS_GENERAL_EXCEPTION("Constant node with empty data domain.", "DAG construction error");
    }
    procedure.emplace_back(std::make_unique<BasicComputeUnitType>());
    return;
}

void parse_string_to_integer(std::string_view integer_view, BasicIntegerType& data) {
    if (integer_view.empty()) {
        throw PUTILS_GENERAL_EXCEPTION("Empty string input.", "parse error");
    }
    std::string integer_str(integer_view);
    data.sign = true;
    if (integer_str.front() == '+') {
        integer_str = integer_str.substr(1);
    } else if (integer_str.front() == '-') {
        data.sign = false;
        integer_str = integer_str.substr(1);
    }
    size_t len = data.len;
    auto arr = data.get_ensured_pointer();
    memset(arr, 0, len * sizeof(BasicIntegerType::ElementType));
    BasicIntegerType::ElementType base = iofun::store_base(data.iobasic);
    BasicIntegerType::ElementType store_digit = 0ull, power = 1ull;
    int p = 0;
    for (int i = integer_str.length() - 1; i >= 0; i--) {
        BasicIntegerType::ElementType digit;
        try {
            digit = iofun::digit_parse(integer_str[i]);
        } PUTILS_CATCH_THROW_GENERAL
        if (digit >= base) {
            std::stringstream ss;
            ss << "Invalid digit: '" << integer_str[i] << "' in base: " << iofun::base_name(data.iobasic);
            throw PUTILS_GENERAL_EXCEPTION(ss.str(), "");
        }
        store_digit += power * digit;
        power *= iofun::io_base(data.iobasic);
        if (power == iofun::store_base(data.iobasic)) {
            if (p >= len) {
                throw PUTILS_GENERAL_EXCEPTION("Integer length limit exceeded.", "parse error");
            }
            arr[p] = store_digit, p++;
            store_digit = 0ull, power = 1ull;
        }
    }
    if (store_digit != 0ull) {
        if (p < len) {
            arr[p] = store_digit;
        } else {
            throw PUTILS_GENERAL_EXCEPTION("Integer length limit exceeded.", "parse error");
        }
    }
    return;
}

void parse_integer_to_stream(std::ostream& stream, const BasicIntegerType& data) noexcept {
    size_t len = data.len;
    auto arr = data.get_pointer();
    if (arr == nullptr) {
        return;
    }
    if (data.sign == false) {
        stream << '-';
    }
    bool none_zero = false;
    for (int i = len - 1; i >= 0; i--) {
        if (arr[i] != 0ull && !none_zero) {
            none_zero = true;
            iofun::write_store_digit_to_stream(stream, data.iobasic, arr[i], false);
        } else if (none_zero) {
            iofun::write_store_digit_to_stream(stream, data.iobasic, arr[i], true);
        }
    }
    if (!none_zero) {
        stream << '0';
        return;
    }
    return;
}

}
