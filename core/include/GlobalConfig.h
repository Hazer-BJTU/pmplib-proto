#pragma once

#include <regex>
#include <cctype>
#include <vector>
#include <variant>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <shared_mutex>

#include "RuntimeLog.h"
#include "FiniteStateMachine.hpp"

#ifdef MPENGINE_CONFIG_LOAD_DEFAULT
#include "DefaultConfigs.hpp"
#endif 

namespace mpengine {

class GlobalConfig;

class ConfigType {
private:
    std::variant<std::monostate, int64_t, double, bool, std::string> data;
    ConfigType& convert(const std::string& str) noexcept;
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
    Type get_or_else(const Type& default_value) const noexcept {
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
    ConfigValueNode(ConfigType&& value);
    ~ConfigValueNode() override;
    ConfigValueNode(const ConfigValueNode&) = delete;
    ConfigValueNode& operator = (const ConfigValueNode&) = delete;
    ConfigValueNode(ConfigValueNode&&) = delete;
    ConfigValueNode& operator = (ConfigValueNode&&) = delete;
};

/**
 * @class GlobalConfig
 * @brief Thread-safe singleton configuration tree for importing/reading program configurations.
 * 
 * This class provides a thread-safe way to manage and access configuration settings
 * throughout the application. It supports:
 * - Loading configurations from files (with JSON-like syntax)
 * - Hierarchical organization of settings
 * - Type-safe value retrieval with fallback defaults
 * - Configuration export functionality
 * 
 * The configuration file format is JSON-like with support for comments. Example:
 * @code
 * {
 *     "Configurations": {
 *         <Comments can be placed like this.>
 *         <All other settings must be contained in the outer domain "Configurations".>
 *         "basic_types": { <It supports basic types.> "integer": 100, "double": 1e-4, "bool": true
 *              <Both "true / false" and "True / False" are OK.>, "string": "100101" <Strings must be enclosed in quotation marks.>,
 *              "string": "version_1.0" <Strings cannot contain spaces or comments; use underscores instead.>,
 *              "inner_domain" <Supports nested dictionaries> : {
 *                  <Nothing here...>
 *              }
 *         },
 *         "list": {"0": 0, "1": 1, "2": 2, "3": 3} <The list uses "0", "1", "2", "3" as keys.>,
 *         <Nested keys are connected using "/".>
 *         "first": {"second": {"third": { "value": 0 <key = "first/second/third/value"> }}}
 *     }
 * }
 * @endcode
 * 
 * @note Key names should avoid comment symbols and spaces (use underscores instead).
 * All configurations must be wrapped in a "Configurations" domain.
 * 
 * Thread Safety:
 * - Singleton access is thread-safe
 * - Configuration reads are concurrent (shared lock)
 * - Configuration writes are exclusive (unique lock)
 * 
 * @warning This class is non-copyable and non-movable (singleton pattern).
 * 
 * @author Hazer
 * @date 2025/8/4
 */

class GlobalConfig {
public:
    using NodePtr = std::shared_ptr<ConfigNode>;
private:
    static const std::regex valid_key;
    static const std::regex comments;
    static const std::regex valid_doc_char;
    static std::string config_filepath;
    static size_t indent;
    static std::mutex setting_lock;
    NodePtr root;
    mutable std::shared_mutex config_lock;
    GlobalConfig();
    ~GlobalConfig();
    static std::string extract_domain(std::string& key) noexcept;
    void recursive_write(
        const std::string& domain_name, 
        const NodePtr& p, 
        std::ostream& stream,
        const size_t indent, 
        size_t layer = 0
    ) const noexcept;
    static std::string_view extract_parse_field(std::string_view config_str, std::string& domain, size_t& p) noexcept;
    void parse_and_set(std::string config_str);
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
    template<typename Type>
    Type get_or_else(const std::string& key, const Type& default_value) const noexcept {
        auto& logger = putils::RuntimeLog::get_global_log();
        std::shared_lock<std::shared_mutex> slock(config_lock);
        NodePtr p = root;
        std::string remain_key = key, domain;
        while(true) {
            domain = extract_domain(remain_key);
            auto dp = std::dynamic_pointer_cast<ConfigDomainNode>(p);
            if (!dp) {
                logger.add("(Configuration): Value type nodes don't have subdomains.");
                return default_value;
            }
            auto it = dp->children.find(domain);
            if (remain_key == "") {
                if (it == dp->children.end()) {
                    logger.add("(Configuration): Key '" + key + "' not found.");
                    return default_value;
                } else {
                    auto vp = std::dynamic_pointer_cast<ConfigValueNode>(it->second);
                    if (!vp) {
                        logger.add("(Configuration): Domain type nodes don't have values.");
                        return default_value;
                    }
                    return vp->value.get_or_else<Type>(default_value);
                }
            } else {
                if (it == dp->children.end()) {
                    logger.add("(Configuration): Key '" + key + "' not found.");
                    return default_value;
                } else {
                    p = it->second;
                }
            }
        }
    }
    void export_all() const noexcept;
    void read_from(std::string filepath = "") noexcept;
};

class ConfigParser: public Automaton {
private:
    std::vector<std::pair<std::string, std::string>> tokens;
    void initialize() noexcept {
        add_node("Initial");
        add_node("KeyField");
        add_node("Colon");
        add_node("Terminal");
        add_node("ValueString");
        add_node("ValueOthers");
        add_node("StartDomain");
        add_node("EndDomain");
        add_node("Comma");
    }
public:
    ConfigParser() {
        initialize();
    }
    ~ConfigParser() override {}
};

}
