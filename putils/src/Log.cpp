#include "Log.h"

namespace putils {

Logger::Logger(): once_flag_buffer(),
                  log_filepath(DEFAULT_LOG_FILE),
                  log_capacity(DEFAULT_LOG_CAPACITY),
                  log_level(DEFAULT_LOG_LEVEL),
                  log_buffer(nullptr),
                  initialized(false),
                  flushing(false),
                  logger_lock() { register_exit(); }
                  
Logger::~Logger() { try { flush(); } catch(...) {} }

void Logger::flush() {
    if (!initialized.load(std::memory_order_acquire)) {
        return;
    }
    if (flushing.exchange(true, std::memory_order_acq_rel)) {
        //Only one thread flushes the buffer.
        std::this_thread::yield();
        return;
    }
    std::lock_guard<std::mutex> lock(logger_lock);
    std::ofstream file_out(log_filepath, std::ios::out | std::ios::app);
    auto guard = ScopeGuard([this, &file_out]() {
        file_out.close();
        flushing.store(false, std::memory_order_release);
    });
    try {
        if (!file_out.is_open()) {
            throw PUTILS_GENERAL_EXCEPTION("Failed to open runtime log file!", "I/O error");
        }
        std::shared_ptr<Entry> entry;
        auto converter = [](Level level) -> const char* {
            switch(level) {
                case Level::INFO: return "Information";
                case Level::WARN: return "Warning";
                case Level::ERROR: return "Fatal error";
                default: return "Others";
            }
        };
        std::stringstream ss;
        while(log_buffer->try_pop(entry)) {
            if (entry && entry->second >= log_level) {
                ss.clear();
                ss << "[" << converter(entry->second) << "]: ";
                ss << "Thread: " << get_local_thread_id() << ", time: " << get_local_time_r() << std::endl;
                ss << entry->first;
                std::string str = ss.str();
                if (str.back() == '\n') {
                    file_out << str;
                } else {
                    file_out << str << std::endl;
                }
            }
        }
    } PUTILS_CATCH_THROW_GENERAL
    return;
}

void Logger::register_exit() noexcept {
    std::set_terminate([]() {
        if (auto eptr = std::current_exception()) {
            try {
                std::rethrow_exception(eptr);
            } catch (const std::exception& e) {
                std::cerr << "Exception occurred!" << std::endl; 
                std::cerr << e.what() << std::endl;
            }
        }
        try {
            ::putils::Logger::get_global_logger().flush();
        } catch(...) {}
        std::abort();
    });
    return;
}

Logger& Logger::get_global_logger() {
    static Logger log_instance;
    return log_instance;
}

void Logger::set(const std::string& filepath, Level level, size_t capacity) noexcept {
    std::lock_guard<std::mutex> lock(logger_lock);
    log_filepath = filepath;
    log_level = level;
    if (!initialized.load(std::memory_order_acquire)) {
        log_capacity = capacity;
    }
    return;
}

void Logger::add(const std::string& log_message, Level level) {
    try {
        std::call_once(once_flag_buffer, [this]() {
            std::lock_guard<std::mutex> lock(logger_lock);
            log_buffer = std::make_unique<LFQ>(log_capacity);
            initialized.store(true, std::memory_order_release);
        });
        while(!log_buffer->try_enqueue(log_message, level)) {
            flush();
        }
    } PUTILS_CATCH_THROW_GENERAL
    return;
}

}
