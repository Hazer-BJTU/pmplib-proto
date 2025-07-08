#include "RuntimeLog.h"

namespace putils {

TerminateCalls::TerminateCalls(): callback_id(0), callbacks(), handler_lock(), executing(false) {
    std::set_terminate([]() {
        auto& handler = ::putils::TerminateCalls::get_terminate_handler();
        handler.execute_all_callbacks();
        std::abort();
    });
    register_callback([this]() {
        if (auto exception_ptr = std::current_exception()) {
            try {
                std::rethrow_exception(exception_ptr);
            } catch (const std::exception& e) {
                std::cerr << "Exception occurred!" << std::endl;
                std::cerr << e.what() << std::endl;
            }
        }
    });
}

TerminateCalls::~TerminateCalls() {}

TerminateCalls& TerminateCalls::get_terminate_handler() noexcept {
    static TerminateCalls terminate_handler_instance;
    return terminate_handler_instance;
}

bool TerminateCalls::remove_callback(size_t remove_idx) noexcept {
    std::lock_guard<std::recursive_mutex> lock(handler_lock);
    auto iterator = std::find_if(
        callbacks.begin(),
        callbacks.end(),
        [remove_idx] (const auto& callback) {
            return callback.second == remove_idx;
        }
    );
    if (iterator != callbacks.end()) {
        callbacks.erase(iterator);
        return true;
    }
    return false;
}

bool TerminateCalls::execute_all_callbacks() noexcept {
    if (executing.exchange(true, std::memory_order_acq_rel)) {
        //Ensure that the function is called only once and by a single thread.
        return true;
    }
    //Block all register and remove operations from other threads.
    std::lock_guard<std::recursive_mutex> lock(handler_lock);
    bool no_exceptions = true;
    for (auto& callback: callbacks) {
        try {
            callback.first();
        } catch(...) { 
            //Ignore all potential exceptions.
            no_exceptions = false;
        }
    }
    return no_exceptions;
}

std::atomic<bool> RuntimeLog::initialized(false);
std::string RuntimeLog::log_filepath("runtime_log.txt");
size_t RuntimeLog::log_capacity = 256;
RuntimeLog::Level RuntimeLog::log_level = RuntimeLog::Level::INFO;
std::mutex RuntimeLog::setting_lock;

RuntimeLog::RuntimeLog(): flushing(false) {
    std::lock_guard<std::mutex> lock(RuntimeLog::setting_lock);
    RuntimeLog::initialized.store(true, std::memory_order_release);
    try {
        log_buffer = std::make_unique<LFQ>(RuntimeLog::log_capacity);
    } PUTILS_CATCH_THROW_GENERAL
    register_exit();
}

RuntimeLog::~RuntimeLog() {
    while(!log_buffer->empty()) {
        try {
            flush();
        } catch(...) {
            //Ignore all potential exceptions.
            std::cerr << "Failed to flush logs during destruction!" << std::endl;
        }
    }
}

void RuntimeLog::register_exit() noexcept {
    auto& handler = TerminateCalls::get_terminate_handler();
    handler.register_callback([this]() { this->~RuntimeLog(); });
    return;
}

bool RuntimeLog::set_global_log(
    const std::string& log_file_path,
    Level log_level,
    size_t log_capacity
) noexcept {
    std::lock_guard<std::mutex> lock(RuntimeLog::setting_lock);
    if (RuntimeLog::initialized.load(std::memory_order_acquire)) {
        /* After an instance is created, its capacity is fixed. 
           Modifying the static member log_capacity will not take effect. */
        RuntimeLog::log_filepath = log_file_path;
        RuntimeLog::log_level = log_level;
        return false;
    }
    RuntimeLog::log_filepath = log_file_path;
    RuntimeLog::log_capacity = log_capacity;
    RuntimeLog::log_level = log_level;
    return true;
}

RuntimeLog& RuntimeLog::get_global_log() noexcept {
    static RuntimeLog log_instance;
    return log_instance;
}

void RuntimeLog::flush() {
    if (flushing.exchange(true, std::memory_order_acq_rel)) {
        /* Only one thread can consume logs (i.e., store them to a file). 
           If multiple threads compete to consume logs, other threads will exit the flush() function.
           After yielding the scheduler, these threads will return to the loop and repeatedly attempt to add logs.
           Thanks to the protection provided by the LockFreeQueue, simultaneous production and consumption of the log buffer are allowed. */
        std::this_thread::yield();
        return;
    }
    std::string filepath;
    Level log_level;
    {
        std::lock_guard<std::mutex> lock(RuntimeLog::setting_lock);
        filepath = RuntimeLog::log_filepath;
        log_level = RuntimeLog::log_level;
    }
    std::ofstream file_out(log_filepath, std::ios::out | std::ios::app);
    auto auard = ScopeGuard([this, &file_out]() {
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
                case Level::WARN: return "  Warning  ";
                case Level::ERROR: return "Fatal error";
                default: return "   Others    ";
            }
        };
        std::stringstream ss;
        while(log_buffer->try_pop(entry)) {
            if (entry && entry->second >= log_level) {
                ss << "[" << converter(entry->second) << "]: ";
                ss << "Thread: " << get_local_thread_id() << ", time: " << get_local_time_r() << std::endl;
                ss << entry->first;
                if (entry->first.back() != '\n') {
                    ss << std::endl;
                }
            }
        }
        file_out << ss.str();
    } PUTILS_CATCH_THROW_GENERAL
    return;
}

void RuntimeLog::add(const std::string& message, Level level) {
    try {
        while(!log_buffer->try_enqueue(message, level)) {
            flush();
        }
    } PUTILS_CATCH_THROW_GENERAL
    return;
}

}
