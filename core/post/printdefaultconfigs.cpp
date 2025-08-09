#include "GlobalConfig.h"

int main(int argc, char** argv) {
    auto& config = mpengine::GlobalConfig::get_global_config();
    config.export_all(argv[argc - 1]);
    return 0;
}
