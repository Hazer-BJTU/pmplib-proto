#include "GlobalConfig.h"

int main() {
    mpengine::GlobalConfig::set_global_config("default_configs.conf", 4);
    mpengine::GlobalConfig::get_global_config().export_all();
    return 0;
}