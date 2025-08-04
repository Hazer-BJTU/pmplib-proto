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

ConfigType& ConfigType::convert(const std::string& str) noexcept {
    size_t pos;
    try {
        int64_t value = std::stoll(str, &pos);
        if (pos == str.length()) {
            data = value;
            return *this;
        }
    } catch(...) {}
    try {
        double value = std::stod(str, &pos);
        if (pos == str.length()) {
            data = value;
            return *this;
        }
    } catch(...) {}
    if (str == "true" || str == "True") {
        data = true;
        return *this;
    }
    if (str == "false" || str == "False") {
        data = false;
        return *this;
    }
    data = str;
    return *this;
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
    } else if (config.get_type() == "string") {
        stream << std::quoted(config.get_or_else<std::string>("")); 
    } else {
        std::visit([&stream](auto&& arg) { stream << arg; }, config.data);
    }
    return stream;
}

ConfigDomainNode::ConfigDomainNode(): children() {}

ConfigDomainNode::~ConfigDomainNode() {}

ConfigValueNode::ConfigValueNode(const ConfigType& value): value(value) {}

ConfigValueNode::ConfigValueNode(ConfigType&& value): value(std::forward<ConfigType>(value)) {}

ConfigValueNode::~ConfigValueNode() {}

const std::regex GlobalConfig::valid_key("[a-zA-Z0-9_.]+(/[a-zA-Z0-9_.]+)*");
const std::regex GlobalConfig::comments("<.*?>");
const std::regex GlobalConfig::valid_doc_char("[a-zA-Z0-9_.\":{,}]+");
std::string GlobalConfig::config_filepath("configurations.conf");
size_t GlobalConfig::indent = 4;
std::atomic<bool> GlobalConfig::initialized = false;
std::mutex GlobalConfig::setting_lock;

GlobalConfig::GlobalConfig(): root(nullptr), config_lock() {
    std::lock_guard<std::mutex> lock(GlobalConfig::setting_lock);
    GlobalConfig::initialized.store(true, std::memory_order_release);
    auto& logger = putils::RuntimeLog::get_global_log();
    std::ifstream file_in(GlobalConfig::config_filepath, std::ios::in);
    if (!file_in.is_open()) {
        logger.add("(Configuration): Failed to open configuration file: " + GlobalConfig::config_filepath + "!", putils::RuntimeLog::Level::WARN);
        root = std::make_shared<ConfigDomainNode>();
    } else {
        std::ostringstream oss;
        oss << file_in.rdbuf();
        try {
            parse_and_set(oss.str());
        } PUTILS_CATCH_THROW_GENERAL
    }
}

GlobalConfig::~GlobalConfig() {}

std::string GlobalConfig::extract_domain(std::string& key) noexcept {
    size_t pos = key.find("/");
    if (pos == std::string::npos) {
        std::string result = key;
        key.clear();
        return result;
    }
    std::string result = key.substr(0, pos);
    key = key.substr(pos + 1);
    return result;
}

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

bool GlobalConfig::insert(const std::string& key, const ConfigType& value) noexcept {
    auto& logger = putils::RuntimeLog::get_global_log();
    if (!std::regex_match(key, GlobalConfig::valid_key)) {
        logger.add("(Configuration): Invalid key: '" + key + "'!");
        return false;
    }
    std::unique_lock<std::shared_mutex> xlock(config_lock);
    NodePtr p = root;
    std::string remain_key = key, domain;
    while(true) {
        domain = extract_domain(remain_key);
        auto dp = std::dynamic_pointer_cast<ConfigDomainNode>(p);
        if (!dp) {
            logger.add("(Configuration): Value type nodes cannot add subdomains.");
            return false;
        }
        auto it = dp->children.find(domain);
        if (remain_key == "") {
            if (it == dp->children.end()) {
                dp->children.insert(std::make_pair(domain, std::make_shared<ConfigValueNode>(value)));
            } else {
                auto vp = std::dynamic_pointer_cast<ConfigValueNode>(it->second);
                if (!vp) {
                    logger.add("(Configuration): Domain type nodes cannot set values.");
                    return false;
                }
                vp->value = value;
            }
            return true;
        } else {
            if (it == dp->children.end()) {
                auto [new_it, inserted] = dp->children.insert(std::make_pair(domain, std::make_shared<ConfigDomainNode>()));
                p = new_it->second;
            } else {
                p = it->second;
            }
        }
    }
}

void GlobalConfig::recursive_write(
    const std::string& domain_name, 
    const NodePtr& p, 
    std::ostream& stream,
    const size_t indent, 
    size_t layer
) const noexcept {
    auto dp = std::dynamic_pointer_cast<ConfigDomainNode>(p);
    if (dp) {
        for (size_t i = 0; i < layer * indent; i++) {
            stream << " ";
        }
        if (!domain_name.empty()) {
            stream << std::quoted(domain_name) << ": {" << std::endl;
        } else {
            stream << "{" << std::endl;
        }
        for (auto it = dp->children.begin(); it != dp->children.end(); it++) {
            recursive_write(it->first, it->second, stream, indent, layer + 1);
            if (std::next(it) != dp->children.end()) {
                stream << ",";
            }
            stream << std::endl;
        }
        for (size_t i = 0; i < layer * indent; i++) {
            stream << " ";
        }
        stream << "}";
        return;
    }
    auto vp = std::dynamic_pointer_cast<ConfigValueNode>(p);
    if (vp) {
        for (size_t i = 0; i < layer * indent; i++) {
            stream << " ";
        }
        stream << std::quoted(domain_name) << ": " << vp->value;
        return;
    }
    return;
}

void GlobalConfig::export_all() const noexcept {
    auto& logger = putils::RuntimeLog::get_global_log();
    std::shared_lock<std::shared_mutex> slock(config_lock);
    std::string filepath;
    size_t indent;
    {
        std::lock_guard<std::mutex> lock(setting_lock);
        filepath = GlobalConfig::config_filepath;
        indent = GlobalConfig::indent;
    }
    std::ofstream file_out(filepath, std::ios::out);
    if (!file_out.is_open()) {
        logger.add("(Configuration): Failed to open configuration file: " + filepath + "!", putils::RuntimeLog::Level::WARN);
        return;
    }
    recursive_write("", root, file_out, indent, 0);
    return;
}

std::string_view GlobalConfig::extract_parse_field(std::string_view config_str, std::string& domain, size_t& p) noexcept {
    domain.clear();
    if (p >= config_str.length()) {
        return "PARSE_OVER";
    }
    while(p < config_str.length()) {
        switch(config_str[p]) {
            case ':' : p++; return "KEY_FIELD";
            case '{' : p++; return "START_DOMAIN";
            case '}' : p++; return "END_DOMAIN";
            case ',' : p++; return "END_FIELD";
            default : domain.push_back(config_str[p]); p++;
        }
    }
    return "UNEXPECTED_EOS";
}

void GlobalConfig::parse_and_set(std::string config_str) {
    //Remove all invisible characters.
    auto new_end = std::remove_if(config_str.begin(), config_str.end(), [] (char c) { return std::isspace(c); });
    config_str.erase(new_end, config_str.end());
    //Remove comments.
    std::regex_replace(config_str, GlobalConfig::comments, "");
    if (config_str.empty() || !std::regex_match(config_str, GlobalConfig::valid_doc_char)) {
        throw PUTILS_GENERAL_EXCEPTION("(Configurations): Wrong configuration file format - invalid characters or comments.", "invalid doc format");
    }
    if (!config_str.empty() && config_str.front() == '{' && config_str.back() == '}') {
        config_str = config_str.substr(1, config_str.length() - 2);
    }
    size_t p = 0;
    std::string domain;
    //Reset config tree.
    root = std::make_shared<ConfigDomainNode>();
    std::string_view state;
    std::stack<NodePtr> node_stack;
    node_stack.push(root);
    while((state = extract_parse_field(config_str, domain, p)) != "PARSE_OVER") {
        auto& node = node_stack.top();
        if (state == "UNEXPECTED_EOS") {
            throw PUTILS_GENERAL_EXCEPTION("(Configurations): Wrong configuration file format - unexpected end of string.", "invalid doc format");
        } else if (state == "KEY_FIELD") {
            if (domain.empty()) {
                throw PUTILS_GENERAL_EXCEPTION("(Configurations): Wrong configuration file format - invalid key field.", "invalid doc format");
            }
            if (domain.front() == '\"' && domain.back() == '\"') {
                domain = domain.substr(1, domain.length() - 2);
            }
            std::string new_domain;
            std::string_view new_state = extract_parse_field(config_str, new_domain, p);
            if (new_state == "START_DOMAIN") {
                auto dnode = std::dynamic_pointer_cast<ConfigDomainNode>(node);
                if (!dnode) {
                    throw PUTILS_GENERAL_EXCEPTION("(Configurations): Value type nodes cannot add subdomains.", "type error");
                }
                auto [new_it, inserted] = dnode->children.insert(std::make_pair(domain, std::make_shared<ConfigDomainNode>()));
                node_stack.push(new_it->second);
            } else if (new_state == "END_FIELD" || new_state == "END_DOMAIN") {
                if (!new_domain.empty()) {
                    auto dnode = std::dynamic_pointer_cast<ConfigDomainNode>(node);
                    if (!dnode) {
                        throw PUTILS_GENERAL_EXCEPTION("(Configurations): Value type nodes cannot add subdomains.", "type error");
                    }
                    if (new_domain.front() == '\"' && new_domain.back() == '\"') {
                        new_domain = new_domain.substr(1, new_domain.length() - 2);
                    }
                    dnode->children.insert(std::make_pair(domain, std::make_shared<ConfigValueNode>(std::move(ConfigType().convert(new_domain)))));
                }
                if (new_state == "END_DOMAIN") {
                    node_stack.pop();
                }
            } else if (new_state == "UNEXPECTED_EOS") {
                throw  PUTILS_GENERAL_EXCEPTION("(Configurations): Wrong configuration file format - unexpected end of string.", "invalid doc format");
            } else if (new_state == "KEY_FIELD") {
                throw PUTILS_GENERAL_EXCEPTION("(Configurations): Wrong configuration file format - empty value field.", "invalid doc format");
            }
        } else if (state == "END_FIELD") {
            continue;
        } else if (state == "END_DOMAIN"){
            node_stack.pop();
        } else {
            throw PUTILS_GENERAL_EXCEPTION("(Configurations): Wrong configuration file format - invalid key field.", "invalid doc format");
        }
    }
    return;
}

}
