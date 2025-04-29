#include "Log.h"

namespace rpc1k {

Log::Log(): file_path(DEFAULT_LOG_FILE), enable_debug(false) {}

Log::~Log() {}

Log& Log::get_global_log() {
    //Meyers' Singleton.
    static Log log_instance;
    return log_instance;
}

void Log::change_file_path(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(log_lock);
    this->file_path = file_path;
    return;
}

void Log::start_debug() {
    std::lock_guard<std::mutex> lock(log_lock);
    enable_debug = true;
    return;
}

void Log::end_debug() {
    std::lock_guard<std::mutex> lock(log_lock);
    enable_debug = false;
    return;
}

void Log::err(
    const std::string& msg, errlevel level, int exitcode,
    const char* file, int line, const char* func
) {
    std::lock_guard<std::mutex> lock(log_lock);
    std::ofstream file_out;
    if (file_path != "%") {
        //If you don't want to write output into files, use "%" as file path.
        file_out.open(file_path, std::ios::app);
    }
    std::string title("");
    if (level == errlevel::DEBUG) {
        if (!enable_debug) {
            //Skip logging if debug mode is not enabled.
            return;
        }
        title.append("[DEBUG] ");
    } else if (level == errlevel::WARNING) {
        title.append("[WARNING] ");
    } else if (level == errlevel::ERROR){
        title.append("[ERROR] ");
    }
    //Get current time.
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    //Write the results.
    std::cout << title << file << ": line " << line << " in " << func << std::endl;
    std::cout << "Msg: " << msg << std::endl;
    if (file_out.is_open()) {
        file_out << title << file << ": line " << line << " in " << func << std::endl;
        file_out << "Msg: " << msg << std::endl;
    }
    //Write log timestamp.
    std::cout << "Timestamp: " << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << std::endl;
    if (file_out.is_open()) {
        file_out << "Timestamp: " << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << std::endl;
    }
    //Write thread info.
    std::cout << "Thread ID: " << std::this_thread::get_id() << std::endl;
    if (file_out.is_open()) {
        file_out << "Thread ID: " << std::this_thread::get_id() << std::endl;
    }
    //If error level reaches ERROR, the program will quit.
    if (level == errlevel::ERROR) {
        std::cout << "Program quited." << std::endl;
        if (file_out.is_open()) {
            file_out << "Program quited." << std::endl;
            file_out.close();
        }
        std::exit(exitcode);
    }
    if (file_out.is_open()) {
        file_out.close();
    }
    return;
}

}
