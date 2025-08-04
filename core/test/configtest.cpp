#include "GlobalConfig.h"

using namespace std;

int main() {
    /*
    auto& config = mpengine::GlobalConfig::get_global_config();
    config.set_global_config("configurations.conf", 4);
    config.insert("models/ResNet/learning_rate", 1e-4);
    config.insert("models/ResNet/input/height", 224);
    config.insert("models/ResNet/input/width", 224);
    config.insert("models/ResNet/train_epochs", 500);
    config.insert("datasets/num_train", 8000);
    config.insert("datasets/num_valid", 1000);
    config.insert("datasets/num_test", 1000);
    config.insert("metrics/0", "Acc");
    config.insert("metrics/1", "mF1");
    config.insert("save_models", true);
    config.export_all();
    */
    mpengine::GlobalConfig::set_global_config("/mnt/d/RPC-1K/conf/configurations.json");
    auto& config = mpengine::GlobalConfig::get_global_config();
    mpengine::GlobalConfig::set_global_config("configurations.json");
    config.export_all();
    return 0;
}