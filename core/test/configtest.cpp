#include "GlobalConfig.h"

int main() {
    auto config_parser = mpengine::ConfigParser();
    std::ifstream file_in("config_test.txt");
    if (!file_in.is_open()) {
        std::cout << "Failed to open the file!" << std::endl;
        return 0;
    }
    std::ostringstream oss;
    oss << file_in.rdbuf();
    std::vector<std::pair<mpengine::ConfigParser::identifier, std::string>> tokens;
    bool successful = config_parser.parse_and_get_tokens(oss.str(), tokens);
    if (successful) {
        std::cout << "Successful: " << std::endl;
        for (auto& [token_type, token_content]: tokens) {
            //std::cout << token_type << ": " << token_content << std::endl;
            ;
        }
    } else {
        std::cout << "Parse failed!" << std::endl;
    }
    return 0;
}