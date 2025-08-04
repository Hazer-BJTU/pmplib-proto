#include "GlobalConfig.h"

using namespace std;

int main() {
    auto& config = mpengine::GlobalConfig::get_global_config();
    config.read_from("input_configurations.conf");
    config.export_all();
    return 0;
}