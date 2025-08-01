#pragma once

#include <variant>
#include <iostream>
#include <string_view>
#include <type_traits>

#include "RuntimeLog.h"

namespace mpengine {

class ConfigType {
    private:
        std::variant<int64_t, double, bool, std::string> data;
    public:
        ConfigType();
        ~ConfigType();
        template<typename Type>
        ConfigType(Type&& value): data(std::forward<Type>(value)) {}
        std::string_view get_type() const noexcept;
        template<typename Type>
        Type get_or_else(Type default_value) const noexcept {
            try {
                return std::get<Type>(data);
            } catch(const std::bad_variant_access& e) {
                return default_value;
            }
        }
        friend std::istream& operator >> (std::istream& stream, ConfigType& config) noexcept;
        friend std::ostream& operator << (std::ostream& stream, const ConfigType& config) noexcept;
};

}
