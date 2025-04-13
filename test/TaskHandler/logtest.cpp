#include "Log.h"

using namespace rpc1k;

int main() {
    auto& log = Log::get_global_log();
    log.start_debug();
    log.err("This is a debug.", errlevel::DEBUG);
    log.err("This is a warning.", errlevel::WARNING);
    log.err("This is an error!", errlevel::ERROR, ERROR_WRONG_ORDER);
    return 0;
}
