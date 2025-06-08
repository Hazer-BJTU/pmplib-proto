#include "Log.h"

using namespace std;

int main() {
    auto& logger = putils::Logger::get_global_logger();
    logger.set("runtime_log.txt", putils::Logger::Level::INFO, 127);
    logger.add("Hello world!");
    return 0;
}