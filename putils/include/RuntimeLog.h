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

class RuntimeLog {
public:
    enum class Level {
        INFO = 0, WARN = 1, ERROR = 2
    };
private:
    using Entry = std::pair<std::string, Level>;
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
    static bool set_global_log(const std::string& log_file_path, size_t log_capacity, Level log_level) noexcept;
    static RuntimeLog& get_global_log() noexcept;
    void flush();
    void add(const std::string& message, Level level = Level::INFO);
};

}
