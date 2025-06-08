#pragma once

#include <iostream>
#include <fstream>
#include <utility>
#include <mutex>

#include "LockFreeQueue.hpp"
#include "GeneralException.h"

#define PUTILS_CATCH_LOG_GENERAL(level)                   \
catch(::putils::GeneralException& e) {                    \
    e.append(__FILE__, __func__);                         \
    auto& logger = ::putils::Logger::get_global_logger(); \
    logger.add(e.what(), level);                          \
} catch(const ::std::exception& e) {                      \
    auto& logger = ::putils::Logger::get_global_logger(); \
    logger.add(e.what(), level);                          \
}                                                         \

#define PUTILS_CATCH_LOG_GENERAL_INFO PUTILS_CATCH_LOG_GENERAL(::putils::Logger::Level::INFO)
#define PUTILS_CATCH_LOG_GENERAL_WARN PUTILS_CATCH_LOG_GENERAL(::putils::Logger::Level::WARN)
#define PUTILS_CATCH_LOG_GENERAL_ERROR PUTILS_CATCH_LOG_GENERAL(::putils::Logger::Level::ERROR)

namespace putils {

/**
 * @class Logger
 * @brief Thread-safe logging utility with asynchronous buffering and configurable log levels.
 *
 * The Logger class provides a thread-safe mechanism for logging messages with different severity levels.
 * It uses a lock-free queue for asynchronous message buffering and supports configurable output file,
 * log level filtering, and buffer capacity. The logger automatically flushes buffered messages
 * when the buffer is full or during program termination.
 *
 * Key Features:
 * - Thread-safe logging operations using a combination of mutexes and atomic operations
 * - Asynchronous message buffering via LockFreeQueue to minimize blocking
 * - Configurable log levels (INFO, WARN, ERROR)
 * - Customizable output file path and buffer capacity
 * - Automatic flush on program termination via std::set_terminate
 * - Exception-safe implementation with RAII resource management
 *
 * Usage:
 * 1. Access the global logger instance via get_global_logger()
 * 2. Optionally configure using set() before first use
 * 3. Add log messages using add() with appropriate level
 * 4. Explicit flush() can be called but isn't required for normal operation
 *
 * The logger provides convenience macros (PUTILS_CATCH_LOG_GENERAL_*) for exception handling
 * that automatically logs caught exceptions with specified severity levels.
 *
 * @note The buffer capacity cannot be changed after initialization
 * @note All methods are thread-safe unless otherwise specified
 * @note The logger is designed to be exception-safe and will attempt to flush during termination
 *
 * @see LockFreeQueue for details on the underlying queue implementation
 * @see GeneralException for exception handling integration
 * @author Hazer
 * @date 2025/6/8
 */

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
    std::string log_filepath; //protected by logger_lock
    size_t log_capacity;      //protected by logger_lock
    Level log_level;          //protected by logger_lock
    std::unique_ptr<LFQ> log_buffer;
    std::atomic<bool> initialized, flushing;
    std::mutex logger_lock;
    Logger();
    ~Logger();
    void register_exit() noexcept;
public:
    Logger(const Logger&) = delete;
    Logger& operator = (const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator = (Logger&&) = delete;
    static Logger& get_global_logger();
    void flush();
    void set(const std::string& filepath, Level level, size_t capacity = 0) noexcept;
    void add(const std::string& log_message, Level level = Level::INFO);
};

}
