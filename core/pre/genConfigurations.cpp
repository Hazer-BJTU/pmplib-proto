#include "GlobalConfig.h"

int main() {
    mpengine::GlobalConfig::set_global_config("configurations.conf");
    auto& config = mpengine::GlobalConfig::get_global_config();
    std::cout << "Start writing default configurations...";
    config.export_all();
    std::cout << "Default configurations have been written.";
    return 0;
}
