#pragma once

#include <iostream>
#include <fstream>
#include <utility>
#include <mutex>

#include "LockFreeQueue.hpp"
#include "GeneralException.h"

namespace putils {

class Logger {
public:
    enum class Level {
        INFO = 0, 
        WARN = 1, 
        ERROR = 2
    };
private:
    using Entry = std::pair<std::string, Level>;
    using LFQ = LockFreeQueue<Entry>;
    static constexpr const char* DEFAULT_LOG_FILE = "runtime_log.txt";
    static constexpr size_t DEFAULT_LOG_CAPACITY = 2048;
    static constexpr Level DEFAULT_LOG_LEVEL = Level::WARN;
    std::once_flag once_flag_buffer;
    std::string log_filepath;
    size_t log_capacity;
    Level log_level;
    std::unique_ptr<LFQ> log_buffer;
    std::atomic<bool> initialized, flushing;
    std::mutex logger_lock;
    Logger();
    ~Logger();
    void flush();
    void register_exit() noexcept;
public:
    Logger(const Logger&) = delete;
    Logger& operator = (const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator = (Logger&&) = delete;
    static Logger& get_global_logger();
    void set(const std::string& filepath, Level level, size_t capacity = 0) noexcept;
    void add(const std::string& log_message, Level level = Level::INFO);
};

}
