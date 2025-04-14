#include "Log.h"
#include <thread>
#include <vector>
#include <string>

int main() {
    __NO_RUNTIME_LOG__;
    __END_LOG_DEBUG__;
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([i] () {
            std::string msg("This is a debug in thread: ");
            msg.append(std::to_string(i)).append(".");
            FREELOG(msg, rpc1k::errlevel::DEBUG);
        });
        threads.emplace_back([i] () {
            std::string msg("This is a warning in thread: ");
            msg.append(std::to_string(i)).append(".");
            FREELOG(msg, rpc1k::errlevel::WARNING);
        });
    }
    for (auto& thread: threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads.clear();
    REDIRECT_LOG("runtime_log.txt");
    __START_LOG_DEBUG__;
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([i] () {
            std::string msg("This is a debug in thread: ");
            msg.append(std::to_string(i)).append(".");
            FREELOG(msg, rpc1k::errlevel::DEBUG);
        });
    }
    for (auto& thread: threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads.clear();
    threads.emplace_back([] () {
        std::string msg("Fatal error!");
        AUTOLOG(msg, rpc1k::errlevel::ERROR, rpc1k::ERROR_UNKNOWN_ERROR);
    });
    for (auto& thread: threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads.clear();
    return 0;
}
