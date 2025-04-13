#include "Log.h"

using namespace rpc1k;

int main() {
    __START_LOG_DEBUG__
    FREELOG("This is a debug.", errlevel::DEBUG);
    FREELOG("This is a warning.", errlevel::WARNING);
    AUTOLOG("This is an error!", errlevel::ERROR, ERROR_WRONG_ORDER);
    return 0;
}
