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

BasicNodeType::BasicNodeType(): data(nullptr) {}

BasicNodeType::~BasicNodeType() {}

BasicTransformation::BasicTransformation(): operand(nullptr) {}

BasicTransformation::~BasicTransformation() {}

BasicBinaryOperation::BasicBinaryOperation(): operand_A(nullptr), operand_B(nullptr) {}

BasicBinaryOperation::~BasicBinaryOperation() {}

}
