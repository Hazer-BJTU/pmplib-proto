#pragma once

#include <algorithm>
#include <iostream>
#include <fstream>
#include <utility>
#include <random>
#include <mutex>

#include "LockFreeQueue.hpp"
#include "GeneralException.h"

#define PUTILS_CATCH_LOG_GENERAL(level)                    \
catch(::putils::GeneralException& e) {                     \
    e.append(__FILE__, __func__);                          \
    auto& logger = ::putils::RuntimeLog::get_global_log(); \
    logger.add(e.what(), level);                           \
} catch(const ::std::exception& e) {                       \
    auto& logger = ::putils::RuntimeLog::get_global_log(); \
    logger.add(e.what(), level);                           \
}                                                          \

#define PUTILS_CATCH_LOG_GENERAL_MSG(msg, level)           \
catch(::putils::GeneralException& e) {                     \
    e.append(__FILE__, __func__);                          \
    auto& logger = ::putils::RuntimeLog::get_global_log(); \
    logger.add(msg, level);                                \
    logger.add(e.what(), level);                           \
} catch(const ::std::exception& e) {                       \
    auto& logger = ::putils::RuntimeLog::get_global_log(); \
    logger.add(msg, level);                                \
    logger.add(e.what(), level);                           \
}                                                          \

#define PUTILS_CATCH_LOG_GENERAL_INFO PUTILS_CATCH_LOG_GENERAL(::putils::RuntimeLog::Level::INFO)
#define PUTILS_CATCH_LOG_GENERAL_WARN PUTILS_CATCH_LOG_GENERAL(::putils::RuntimeLog::Level::WARN)
#define PUTILS_CATCH_LOG_GENERAL_ERROR PUTILS_CATCH_LOG_GENERAL(::putils::RuntimeLog::Level::ERROR)

namespace putils {

class TerminateCalls {
public:
    using CallbackList = std::vector<std::pair<std::function<void()>, size_t>>;
private:
    size_t callback_id;
    CallbackList callbacks;
    std::atomic<bool> executing;
    std::recursive_mutex handler_lock;
    TerminateCalls();
    ~TerminateCalls();
public:
    TerminateCalls(const TerminateCalls&) = delete;
    TerminateCalls& operator = (const TerminateCalls&) = delete;
    TerminateCalls(TerminateCalls&&) = delete;
    TerminateCalls& operator = (TerminateCalls&&) = delete;
    static TerminateCalls& get_terminate_handler() noexcept;
    template<typename Lambda>
    size_t register_callback(Lambda&& callback) noexcept {
        std::lock_guard<std::recursive_mutex> lock(handler_lock);
        callbacks.emplace_back(std::forward<Lambda>(callback), callback_id++);
        return callback_id - 1;
    }
    bool remove_callback(size_t remove_idx) noexcept;
    bool execute_all_callbacks() noexcept;
};

/**
 * @class RuntimeLog
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
 * @date 2025/7/5
 */

class RuntimeLog {
public:
    enum class Level {
        INFO = 0, WARN = 1, ERROR = 2
    };
private:
    /* using Entry = std::pair<std::string, Level>; */
    struct Entry {
        std::string msg;
        Level lvl;
        std::string tid;
        std::string tim;
    };
    using LFQ = LockFreeQueue<Entry>;
    static std::atomic<bool> initialized;
    static std::string log_filepath; //protected by setting_lock
    static size_t log_capacity;      //protected by setting_lock
    static Level log_level;          //protected by setting_lock
    static std::mutex setting_lock;
    std::unique_ptr<LFQ> log_buffer;
    std::atomic<bool> flushing;
    RuntimeLog();
    ~RuntimeLog();
    void register_exit() noexcept;
public:
    RuntimeLog(const RuntimeLog&) = delete;
    RuntimeLog& operator = (const RuntimeLog&) = delete;
    RuntimeLog(RuntimeLog&&) = delete;
    RuntimeLog& operator = (RuntimeLog&&) = delete;
    static bool set_global_log(const std::string& log_file_path, Level log_level, size_t log_capacity = RuntimeLog::log_capacity) noexcept;
    static RuntimeLog& get_global_log() noexcept;
    void flush();
    void add(const std::string& message, Level level = Level::INFO);
};

}
