#include "Log.h"

using namespace std;

int main() {
    auto& logger = putils::Logger::get_global_logger();
    logger.add("Hello world!", putils::Logger::Level::WARN);
    return 0;
}