#include "GlobalConfig.h"

namespace mpengine {

ConfigType::ConfigType(): data(false) {};

ConfigType::~ConfigType() {};

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

std::istream& operator >> (std::istream& stream, ConfigType& config) noexcept {
    std::string str;
    stream >> str;
    size_t pos;
    try {
        int64_t value = std::stoll(str, &pos);
        if (pos == str.length()) {
            config.data = value;
            return stream;
        }
    } catch(...) {}
    try {
        double value = std::stod(str, &pos);
        if (pos == str.length()) {
            config.data = value;
            return stream;
        }
    } catch(...) {}
    if (str == "true" || str == "True") {
        config.data = true;
        return stream;
    }
    if (str == "false" || str == "False") {
        config.data = false;
        return stream;
    }
    config.data = str;
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

}
