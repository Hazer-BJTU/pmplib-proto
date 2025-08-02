#pragma once

#include <regex>
#include <variant>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <shared_mutex>

#include "RuntimeLog.h"

namespace mpengine {

class GlobalConfig;

class ConfigType {
private:
    std::variant<int64_t, double, bool, std::string> data;
    void convert(const std::string& str) noexcept;
    friend GlobalConfig;
public:
    ConfigType();
    ~ConfigType();
    ConfigType(const ConfigType&) = default;
    ConfigType& operator = (const ConfigType&) = default;
    ConfigType(ConfigType&&) = default;
    ConfigType& operator = (ConfigType&&) = default;
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

class ConfigNode {
public:
    using NodePtr = std::shared_ptr<ConfigNode>;
public:
    ConfigNode() = default;
    virtual ~ConfigNode() = default;
};

class ConfigDomainNode: public ConfigNode {
public:
    using NodePtr = std::shared_ptr<ConfigNode>;
private:
    std::map<std::string, NodePtr> children;
    friend GlobalConfig;
public:
    ConfigDomainNode();
    ~ConfigDomainNode() override;
    ConfigDomainNode(const ConfigDomainNode&) = delete;
    ConfigDomainNode& operator = (const ConfigDomainNode&) = delete;
    ConfigDomainNode(ConfigDomainNode&&) = delete;
    ConfigDomainNode& operator = (ConfigDomainNode&&) = delete;
};

class ConfigValueNode: public ConfigNode {
public:
    using NodePtr = std::shared_ptr<ConfigNode>;
private:
    ConfigType value;
    friend GlobalConfig;
public:
    ConfigValueNode(const ConfigType& value);
    ~ConfigValueNode() override;
    ConfigValueNode(const ConfigValueNode&) = delete;
    ConfigValueNode& operator = (const ConfigValueNode&) = delete;
    ConfigValueNode(ConfigValueNode&&) = delete;
    ConfigValueNode& operator = (ConfigValueNode&&) = delete;
};

class GlobalConfig {
public:
    using NodePtr = std::shared_ptr<ConfigNode>;
private:
    static const std::regex valid_key;
    static std::string config_filepath;
    static size_t indent;
    static std::atomic<bool> initialized;
    static std::mutex setting_lock;
    NodePtr root;
    mutable std::shared_mutex config_lock;
    GlobalConfig();
    ~GlobalConfig();
    static std::string extract_domain(std::string& key) noexcept; 
public:
    GlobalConfig(const GlobalConfig&) = delete;
    GlobalConfig& operator = (const GlobalConfig&) = delete;
    GlobalConfig(GlobalConfig&&) = delete;
    GlobalConfig& operator = (GlobalConfig&&) = delete;
    static bool set_global_config(
        const std::string& config_filepath = GlobalConfig::config_filepath,
        size_t indent = GlobalConfig::indent
    ) noexcept;
    static GlobalConfig& get_global_config() noexcept;
    bool insert(const std::string& key, const ConfigType& value) noexcept;
};

}
