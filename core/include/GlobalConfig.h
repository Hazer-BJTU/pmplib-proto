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

// #ifdef MPENGINE_CONFIG_LOAD_DEFAULT
// #include "DefaultConfigs.hpp"
// #endif 

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
    void parse_and_set(const std::string& config_str);
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
    void export_all(const std::string& input_filepath = "") const noexcept;
    void read_from(const std::string& input_filepath = "") noexcept;
};

class ConfigParser: public Automaton {
public:
    enum class identifier { bracket, key, value };
private:
    std::string matched;
    std::vector<std::pair<identifier, std::string>> tokens;
    void initialize() {
        const bool stop_advance = true, accepted_state = true;
        const std::string cs_value = cs::concate(cs::alphanumeric, "+-.");
        add_node("Ready");
        add_node("Key");
        add_node("KeyEnd");
        add_node("Colon");
        add_node("ValueString");
        add_node("ValueOthers");
        add_node("ValueEnd", accepted_state);
        add_transition("Ready", "Ready", cs::whitespace);
        add_transition("Ready", "Ready", '{', [this] { tokens.emplace_back(identifier::bracket, "{"); });
        add_transition("Ready", "ValueEnd", '}', [this] { tokens.emplace_back(identifier::bracket, "}"); });
        add_transition("Ready", "Key", '\"', [this] { matched.clear(); });
        add_transition("Key", "Key", cs::except(cs::text, "\""), [this] { matched.push_back(_current_event); });
        add_transition("Key", "KeyEnd", '\"', [this] { tokens.emplace_back(identifier::key, matched); });
        add_transition("KeyEnd", "KeyEnd", cs::whitespace);
        add_transition("KeyEnd", "Colon", ':');
        add_transition("Colon", "Colon", cs::whitespace);
        add_transition("Colon", "ValueString", '\"', [this] { matched.clear(); matched.push_back(_current_event); });
        add_transition("Colon", "ValueOthers", cs_value, [this] { matched.clear(); matched.push_back(_current_event); });
        add_transition("Colon", "Ready", '{', [this] { tokens.emplace_back(identifier::bracket, "{"); });
        add_transition("ValueString", "ValueString", cs::except(cs::text, "\""), [this] { matched.push_back(_current_event); });
        add_transition("ValueString", "ValueEnd", "\"", [this] { matched.push_back(_current_event); tokens.emplace_back(identifier::value, matched); });
        add_transition("ValueOthers", "ValueOthers", cs_value, [this] { matched.push_back(_current_event); });
        add_transition("ValueOthers", "ValueEnd", cs::except(cs::any, cs_value), [this] { tokens.emplace_back(identifier::value, matched); }, stop_advance);
        add_transition("ValueEnd", "ValueEnd", cs::whitespace);
        add_transition("ValueEnd", "ValueEnd", '}', [this] { tokens.emplace_back(identifier::bracket, "}"); });
        add_transition("ValueEnd", "Ready", ',');
        set_starting("Ready");
    }
public:
    ConfigParser(): matched(), tokens() {
        initialize();
    }
    ~ConfigParser() override {}
    bool parse_and_get_tokens(const std::string& configs, std::vector<std::pair<identifier, std::string>>& tokens_out) {
        reset();
        tokens.clear();
        try {
            steps(configs);
        } PUTILS_CATCH_THROW_GENERAL
        if (!accepted()) {
            return false;
        }
        tokens_out = std::move(tokens);
        return true;
    }
};

}
