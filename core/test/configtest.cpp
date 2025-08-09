#include "GlobalConfig.h"

int main() {
    auto& config = mpengine::GlobalConfig::get_global_config();
    mpengine::GlobalConfig::set_global_config("old_configurations.json");
    config.export_all();
    config.read_from("config_test.txt");
    mpengine::GlobalConfig::set_global_config("new_configurations.json");
    config.export_all();
    return 0;
}