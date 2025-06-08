#pragma once

#include <functional>
#include <exception>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <memory>

#include <execinfo.h>
#include <cxxabi.h>
#include <time.h>

#define PUTILS_GENERAL_EXCEPTION(msg, err) ::putils::GeneralException(msg, err, __FILE__, __func__)

#define PUTILS_CATCH_THROW_GENERAL     \
catch(::putils::GeneralException& e) { \
    e.append(__FILE__, __func__);      \
    throw e;                           \
} catch(const ::std::exception& e) {   \
    throw PUTILS_GENERAL_EXCEPTION(    \
        e.what(), "std exception"      \
    );                                 \
}                                      \

namespace putils {

#ifdef PUTILS_GENERAL_EXCEPTION_KNOWN_ONLY
    inline constexpr bool ignore_unknown = true;
#else
    inline constexpr bool ignore_unknown = false;
#endif

template<typename Lambda>
class ScopeGuard {
private:
    Lambda callback;
    bool active;
public:
    ScopeGuard(Lambda&& target): callback(std::forward<Lambda>(target)), active(true) {}
    ~ScopeGuard() {
        if (active) {
            try {
                callback();
            } catch(...) {}
        }
    }
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator = (const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&& sg) noexcept: callback(std::move(sg.callback)), active(sg.active) {
        sg.dismiss();
    }
    ScopeGuard& operator = (ScopeGuard&& sg) noexcept {
        callback = std::move(sg.callback);
        active = sg.active;
        sg.dismiss();
    }
    void dismiss() noexcept { 
        active = false; 
    }
};

std::string get_local_time_r() noexcept;
std::string get_local_thread_id() noexcept;

/**
 * @class GeneralException
 * @brief An exception class that captures and stores call stack information.
 * 
 * This exception class extends standard exception functionality by:
 * - Capturing the call stack at the point where the exception is thrown
 * - Storing contextual information (file, function, message)
 * - Providing formatted output with complete error details and backtrace
 * 
 * The class works with two companion macros:
 * - GENERAL_EXCEPTION(msg, details): Creates an exception with message and details
 * - CATCH_THROW_GENERAL: Catches any exception, wraps it in GeneralException and rethrows
 * 
 * The output format includes:
 * 1. Original error location (file and function)
 * 2. Error message and details
 * 3. Thread ID and timestamp
 * 4. Propagation path (which functions the exception passed through)
 * 5. Full symbolic backtrace with addresses and locations
 * 
 * @note Requires stack trace functionality (like libunwind or backtrace)
 * @note Macros assume exception handling is done in C++ style
 * 
 * @example
 * // Throwing:
 * throw GENERAL_EXCEPTION("Calculation failed", "Division by zero");
 * 
 * // Catching and propagating:
 * try { someFunction(); }
 * CATCH_THROW_GENERAL
 * 
 * @see GENERAL_EXCEPTION
 * @see CATCH_THROW_GENERAL
 * 
 * @author Hazer
 * @date 2025/6/3
 */

class GeneralException: public std::exception {
private:
    static constexpr size_t MAX_STACK_LENGTH = 128;
    static constexpr size_t MAX_FUNCTION_NAME = 128;
    std::string error_type, final_msg;
    std::vector<std::string> messages, backtraces;
    std::string process_stack_trace(const char* stack_str) noexcept;
public:
    explicit GeneralException(
        const std::string& msg, 
        const std::string& err, 
        const std::string& file,
        const std::string& func
    );
    GeneralException(const GeneralException&) = default;
    GeneralException& operator = (const GeneralException&) = default;
    GeneralException(GeneralException&&) = default;
    GeneralException& operator = (GeneralException&&) = default;
    const char* get_final_msg() noexcept;
    const char* what() const noexcept override;
    const char* type() const noexcept;
    int append(const std::string& file, const std::string& func) noexcept;
    int append(const std::string& others) noexcept;
};

}
