#include "Basics.h"

namespace mpengine {

BasicIntegerType::BasicIntegerType(size_t log_len): sign(1), log_len(log_len), len(0) {
    auto& memorypool = putils::MemoryPool::get_global_memorypool();
    static const size_t min_log_length = GlobalConfig::get_global_config().get_or_else<int64_t>(
        "Configurations/core/BasicIntegerType/limits/min_log_length", 0ll
    );
    static const size_t max_log_length = GlobalConfig::get_global_config().get_or_else<int64_t>(
        "Configurations/core/BasicIntegerType/limits/max_log_length", 0ll
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
    try {
        data = memorypool.allocate(len * sizeof(ElementType));
    } PUTILS_CATCH_THROW_GENERAL
}

BasicIntegerType::~BasicIntegerType() {
    putils::release(data);
}

BasicIntegerType::ElementType* BasicIntegerType::get_pointer() const noexcept {
    return data->get<ElementType>();
}

BasicComputeUnitType::BasicComputeUnitType(): forward_calls() {}

BasicComputeUnitType::~BasicComputeUnitType() {}

void BasicComputeUnitType::dependency_notice() {}

void BasicComputeUnitType::forward() {}

void BasicComputeUnitType::add_dependency(BasicComputeUnitType& predecessor) {}

BasicNodeType::BasicNodeType(): data(nullptr), next(nullptr), procedure() {}

BasicNodeType::~BasicNodeType() {}

BasicTransformation::BasicTransformation(): operand(nullptr) {}

BasicTransformation::~BasicTransformation() {}

BasicBinaryOperation::BasicBinaryOperation(): operand_A(nullptr), operand_B(nullptr) {}

BasicBinaryOperation::~BasicBinaryOperation() {}

ConstantNode::ConstantNode(bool referenced): referenced(referenced) {}

ConstantNode::ConstantNode(const BasicNodeType& node, bool referenced): referenced(referenced) {
    data = node.data;
}

ConstantNode::~ConstantNode() {}

void parse_string_to_integer(std::string_view integer_view, BasicIntegerType& data) {
    if (integer_view.empty()) {
        throw PUTILS_GENERAL_EXCEPTION("Empty string.", "parse error");
    }
    std::string integer_str{integer_view};
    data.sign = true;
    if (integer_str.front() == '+') {
        integer_str = integer_str.substr(1);
    } else if (integer_str.front() == '-') {
        data.sign = false;
        integer_str = integer_str.substr(1);
    }
    size_t len = data.len;
    BasicIntegerType::ElementType* arr = data.get_pointer();
    memset(arr, 0, len * sizeof(BasicIntegerType::ElementType));
    int p = 0;
    BasicIntegerType::ElementType element = 0, power = 1;
    for (int i = integer_str.length() - 1; i >= 0; i--) {
        if (integer_str[i] < '0' || integer_str[i] > '9') {
            std::stringstream ss;
            ss << "Invalid character in integer: '" << integer_str[i] << "'!";
            throw PUTILS_GENERAL_EXCEPTION(ss.str(), "parse error");
        }
        element = element + power * (integer_str[i] - '0');
        power *= BasicIntegerType::BB;
        if (power == BasicIntegerType::BASE) {
            if (p >= len) {
                throw PUTILS_GENERAL_EXCEPTION("Length limit exceeded.", "parse error");
            }
            arr[p] = element, p++;
            element = 0, power = 1;
        }
    }
    if (element != 0) {
        if (p < len) {
            arr[p] = element;
        } else {
            throw PUTILS_GENERAL_EXCEPTION("Length limit exceeded.", "parse error");
        }
    }
}

std::string parse_integer_to_string(const BasicIntegerType& data) noexcept {
    std::stringstream ss;
    size_t len = data.len;
    BasicIntegerType::ElementType* arr = data.get_pointer();
    if (data.sign == false) {
        ss << '-';
    }
    bool none_zero = false;
    for (int i = len - 1; i >= 0; i--) {
        if (arr[i] != 0ull) {
            none_zero = true;
        }
        if (none_zero) {
            ss << arr[i];
        }
    }
    if (!none_zero) {
        return "0";
    }
    return ss.str();
}

}
