#include "GlobalConfig.h"

using namespace std;

int main() {
    auto& config = mpengine::GlobalConfig::get_global_config();
    config.export_all();
    config.read_from("/mnt/d/RPC-1K/conf/configurations.conf");
    std::cout << "Num samples for training: " << config.get_or_else<int64_t>("Configurations/datasets/num_train", 0) << std::endl;
    config.insert("Configurations/datasets/num_train", 12000);
    config.insert("Configurations/metrics/2", "precision");
    config.insert("Configurations/metrics/3", "recall");
    config.insert("Configurations/cache_datasets", false);
    mpengine::GlobalConfig::set_global_config("new_configurations.conf");
    config.export_all();
    return 0;
}