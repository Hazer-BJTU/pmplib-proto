#include "GlobalConfig.h"

namespace mpengine {

ConfigType::ConfigType(): data(false) {}

ConfigType::~ConfigType() {}

std::string_view ConfigType::get_type() const noexcept {
    if (std::holds_alternative<int64_t>(data)) {
        return "long long";
    } else if (std::holds_alternative<double>(data)) {
        return "double";
    } else if (std::holds_alternative<bool>(data)) {
        return "bool";
    } else if (std::holds_alternative<std::string>(data)) {
        return "string";
    }
    return "unknown";
}

void ConfigType::convert(const std::string& str) noexcept {
    size_t pos;
    try {
        int64_t value = std::stoll(str, &pos);
        if (pos == str.length()) {
            data = value;
            return;
        }
    } catch(...) {}
    try {
        double value = std::stod(str, &pos);
        if (pos == str.length()) {
            data = value;
            return;
        }
    } catch(...) {}
    if (str == "true" || str == "True") {
        data = true;
        return;
    }
    if (str == "false" || str == "False") {
        data = false;
        return;
    }
    data = str;
    return;
}

std::istream& operator >> (std::istream& stream, ConfigType& config) noexcept {
    std::string str;
    stream >> str;
    config.convert(str);
    return stream;
}

std::ostream& operator << (std::ostream& stream, const ConfigType& config) noexcept {
    if (config.get_type() == "bool") {
        stream << (config.get_or_else<bool>(false) ? "true" : "false");
    } else {
        std::visit([&stream](auto&& arg) { stream << arg; }, config.data);
    }
    return stream;
}

ConfigDomainNode::ConfigDomainNode(): children() {}

ConfigDomainNode::~ConfigDomainNode() {}

ConfigValueNode::ConfigValueNode(const ConfigType& value): value(value) {}

ConfigValueNode::~ConfigValueNode() {}

const std::regex GlobalConfig::valid_key("[a-zA-Z0-9_.]+(/[a-zA-Z0-9_.]+)*");
std::string GlobalConfig::config_filepath("configurations.conf");
size_t GlobalConfig::indent = 4;
std::atomic<bool> GlobalConfig::initialized = false;
std::mutex GlobalConfig::setting_lock;

GlobalConfig::GlobalConfig() {
    std::lock_guard<std::mutex> lock(GlobalConfig::setting_lock);
    GlobalConfig::initialized.store(true, std::memory_order_release);
}

GlobalConfig::~GlobalConfig() {}

bool GlobalConfig::set_global_config(const std::string& config_filepath, size_t indent) noexcept {
    auto& logger = putils::RuntimeLog::get_global_log();
    std::lock_guard<std::mutex> lock(GlobalConfig::setting_lock);
    if (indent == 0) {
        logger.add("(Configuration): All numeric arguments must be positive integers.");
        return false;
    }
    GlobalConfig::config_filepath = config_filepath;
    GlobalConfig::indent = indent;
    return true;
}

GlobalConfig& GlobalConfig::get_global_config() noexcept {
    static GlobalConfig config_instance;
    return config_instance;
}

}
