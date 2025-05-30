#pragma once

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

#define GENERAL_EXCEPTION(msg, err) ::putils::GeneralException(msg, err, __FILE__, __func__)

#define CATCH_THROW_GENERAL            \
catch(::putils::GeneralException& e) { \
    e.append(__FILE__, __func__);      \
    throw e;                           \
} catch(::std::exception& e) {         \
    throw GENERAL_EXCEPTION(           \
        e.what(), "std exception"      \
    );                                 \
}                                      \

namespace putils {

class GeneralException: public std::exception {
private:
    static constexpr int MAX_STACK_LENGTH = 128;
    static constexpr int MAX_FUNCTION_NAME = 128;
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
    const char* get_final_msg() noexcept;
    const char* what() const noexcept override;
    const char* type() const noexcept;
    int append(const std::string& file, const std::string& func) noexcept;
    int append(const std::string& others) noexcept;
};

}
