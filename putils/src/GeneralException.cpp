#include "GeneralException.h"

namespace putils {

GeneralException::GeneralException(
    const std::string& msg, 
    const std::string& err, 
    const std::string& file,
    const std::string& func
): messages(), error_type(err), final_msg("") {
    auto threadId = std::this_thread::get_id();
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::string threadStr = (std::stringstream() << threadId).str();
    std::tm tm;
    localtime_r(&now_time, &tm);
    std::string timeStr = (std::stringstream() << std::put_time(&tm, "%Y-%m-%d %X")).str();
    std::string head = "In file: " + file + ", in function: " + func;
    std::string body = "Error: " + error_type + ", " + msg;
    std::string info = "Thread: " + threadStr + ", time: " + timeStr;
    messages.push_back(head);
    messages.push_back(body);
    messages.push_back(info);
    //Add full stacktrace.
    void* stack_addr[MAX_STACK_LENGTH];
    int num_frames = backtrace(stack_addr, MAX_STACK_LENGTH);
    char** stack_str = backtrace_symbols(stack_addr, num_frames);
    backtraces.push_back("Full backtrace: ");
    for (int i = 0; i < num_frames; i++) {
        backtraces.push_back(process_stack_trace(stack_str[i]));
    }
    free(stack_str);
}

std::string GeneralException::process_stack_trace(const char* stack_str) noexcept {
    std::string symbol_str(stack_str);
    size_t begin_name_offset, end_name_offset, end_offset_offset, begin_addr_offset, end_addr_offset;
    begin_name_offset = symbol_str.find('(');
    end_name_offset = symbol_str.find('+');
    end_offset_offset = symbol_str.find(')');
    begin_addr_offset = symbol_str.find('[');
    end_addr_offset = symbol_str.find(']');
    if (
        begin_name_offset != std::string::npos &&
        end_name_offset != std::string::npos &&
        end_offset_offset != std::string::npos &&
        begin_addr_offset != std::string::npos &&
        end_addr_offset != std::string::npos
    ) {
        std::string filename = symbol_str.substr(0, begin_name_offset);
        std::string funcname = symbol_str.substr(begin_name_offset + 1, end_name_offset - begin_name_offset - 1);
        std::string offset = symbol_str.substr(end_name_offset, end_offset_offset - end_name_offset);
        std::string address = symbol_str.substr(begin_addr_offset + 1, end_addr_offset - begin_addr_offset - 1);
        //Parse function name.
        if (funcname != "") {
            int status = 0;
            std::unique_ptr<char, decltype(&std::free)> demangled_name(
                abi::__cxa_demangle(funcname.c_str(), nullptr, nullptr, &status),
                &std::free
            );
            if (status == 0 && demangled_name) {
                funcname = demangled_name.get();
                if (funcname.length() > MAX_FUNCTION_NAME) {
                    int cut = funcname.find('(');
                    funcname = funcname.substr(0, cut) + "(...)";
                }
            }
        } else {
            funcname = "unknown";
        }
        std::stringstream ss;
        ss << "[" << address << "] " << funcname << " " << offset << " in file " << filename;
        return ss.str();
    }
    return symbol_str;
}

const char* GeneralException::get_final_msg() noexcept {
    if (final_msg != "") {
        return final_msg.c_str();
    }
    //Start formatting output.
    std::stringstream ss;
    int line_width = 0;
    for (auto& str: messages) {
        line_width = std::max(line_width, (int)str.length());
    }
    for (auto& str: backtraces) {
        line_width = std::max(line_width, (int)str.length());
    }
    for (auto& str: messages) {
        ss << "-- " << std::left << std::setw(line_width) << str << " --" << std::endl;
    }
    for (auto& str: backtraces) {
        ss << "-- " << std::left << std::setw(line_width) << str << " --" << std::endl;
    }
    final_msg = ss.str();
    return final_msg.c_str();
}

const char* GeneralException::what() const noexcept {
    if (final_msg == "") {
        return const_cast<GeneralException*>(this)->get_final_msg();
    }
    return final_msg.c_str();
}

const char* GeneralException::type() const noexcept {
    return error_type.c_str();
}

int GeneralException::append(const std::string& file, const std::string& func) noexcept {
    std::string head = "From file: " + file + ", in function: " + func;
    messages.push_back(head);
    final_msg = "";
    return messages.size();
}

int GeneralException::append(const std::string& others) noexcept {
    messages.push_back(others);
    final_msg = "";
    return messages.size();
}

}
