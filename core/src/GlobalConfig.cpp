#include "GlobalConfig.h"

namespace mpengine {

ConfigType::ConfigType(): data(std::monostate {}) {}

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
    if (!str.empty() && str.front() == '\"' && str.back() == '\"') {
        data = str.substr(1, str.length() - 2);
        return *this;
    }
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

const std::regex GlobalConfig::valid_key("[a-zA-Z0-9-_.]+(/[a-zA-Z0-9-_.]+)*");
std::string GlobalConfig::config_filepath("configurations.conf");
size_t GlobalConfig::indent = 4;
std::mutex GlobalConfig::setting_lock;

GlobalConfig::GlobalConfig(): root(nullptr), config_lock() {
    #ifdef MPENGINE_CONFIG_LOAD_DEFAULT
        try {
            read_from(MPENGINE_DEFAULT_CONFIG_PATH);
            // parse_and_set(std::string{MPENGINE_DEFAULT_CONFIGURATIONS_STRING});
        } catch(...) {
            root = std::make_shared<ConfigDomainNode>();
        }
    #else
        root = std::make_shared<ConfigDomainNode>();
    #endif
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

void GlobalConfig::export_all(const std::string& input_filepath) const noexcept {
    auto& logger = putils::RuntimeLog::get_global_log();
    std::shared_lock<std::shared_mutex> slock(config_lock);
    std::string filepath;
    size_t indent;
    {
        std::lock_guard<std::mutex> lock(GlobalConfig::setting_lock);
        if (input_filepath == "") {
            filepath = GlobalConfig::config_filepath;
        } else {
            filepath = input_filepath;
        }
        indent = GlobalConfig::indent;
    }
    std::ofstream file_out(filepath, std::ios::out);
    if (!file_out.is_open()) {
        logger.add("(Configuration): Failed to open configuration file: " + filepath + "!", putils::RuntimeLog::Level::WARN);
        return;
    }
    recursive_write("", root, file_out, indent, 0);
    file_out.close();
    return;
}

void GlobalConfig::parse_and_set(const std::string& config_str) {
    static ConfigParser config_parser;
    std::vector<std::pair<ConfigParser::identifier, std::string>> tokens;
    try {
        bool successful = config_parser.parse_and_get_tokens(config_str, tokens);
    } PUTILS_CATCH_THROW_GENERAL
    // if (!successful) {
    //    throw PUTILS_GENERAL_EXCEPTION("(Configurations):  Wrong configuration file format.", "invalid config format");
    // }
    std::stack<NodePtr> node_stack;
    root = std::make_shared<ConfigDomainNode>();
    node_stack.push(root);
    auto it = tokens.begin();
    int brackets = 0;
    if (it != tokens.end() && it->first == ConfigParser::identifier::bracket && it->second == "{") {
        brackets++;
        it++;
    }
    while(it != tokens.end()) {
        auto& node = node_stack.top();
        if (it->first == ConfigParser::identifier::bracket) {
            if (it->second == "}") {
                brackets--;
                it++;
                node_stack.pop();
                if (brackets == 0) {
                    break;
                }
            } else if (it->second == "{") {
                throw PUTILS_GENERAL_EXCEPTION("(Configurations): Missing key declaration.", "invalid config format");
            }
        } else if (it->first == ConfigParser::identifier::key) {
            std::string key_name = it->second; it++;
            if (it == tokens.end()) {
                throw PUTILS_GENERAL_EXCEPTION("(Configurations):  Isolated key declaration.", "invalid config format");
            }
            auto dnode = std::dynamic_pointer_cast<ConfigDomainNode>(node);
            if (!dnode) {
                throw PUTILS_GENERAL_EXCEPTION("(Configurations): Value type nodes cannot add subdomains.", "type error");
            }
            if (it->first == ConfigParser::identifier::bracket) {
                brackets++;
                auto [new_node, inserted] = dnode->children.insert(std::make_pair(key_name, std::make_shared<ConfigDomainNode>()));
                node_stack.push(new_node->second);
            } else if (it->first == ConfigParser::identifier::value) {
                dnode->children.insert(std::make_pair(key_name, std::make_shared<ConfigValueNode>(std::move(ConfigType().convert(it->second)))));
            }
            it++;
        }
    }
    return;
}

void GlobalConfig::read_from(const std::string& input_filepath) noexcept {
    auto& logger = putils::RuntimeLog::get_global_log();
    std::string filepath;
    if (input_filepath == "") {
        std::lock_guard<std::mutex> lock(GlobalConfig::setting_lock);
        filepath = GlobalConfig::config_filepath;
    } else {
        filepath = input_filepath;
    }
    std::ifstream file_in(filepath, std::ios::in);
    if (!file_in.is_open()) {
        logger.add("(Configuration): Failed to open configuration file: " + filepath + "!", putils::RuntimeLog::Level::WARN);
        return;
    }
    std::unique_lock<std::shared_mutex> xlock(config_lock);
    std::ostringstream oss;
    oss << file_in.rdbuf();
    try {
        parse_and_set(oss.str());
    } PUTILS_CATCH_LOG_GENERAL_MSG(
        "(Configurations): Parse failed! Configurations may be incomplete!",
        putils::RuntimeLog::Level::WARN
    )
    file_in.close();
}

}
