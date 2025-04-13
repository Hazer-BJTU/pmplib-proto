#pragma once

#include <cstdlib>
#include <thread>
#include <mutex>
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace rpc1k {

/*
 * @brief A simple logging class for multi-threaded environments.
 * 
 * This logger helps debug and handle errors that cannot be caught by try-catch blocks.
 * It supports three log levels with different behaviors:
 *   - [DEBUG]:   Only shown in debug mode
 *   - [WARNING]: Shown in all modes (does not terminate program)
 *   - [ERROR]:   Prints error message and terminates the process
 * 
 * Log entries include:
 *   - Timestamp
 *   - Thread information
 *   - Log level
 *   - Custom message
 * 
 * By default writes to "runtime_log.txt".
 * Set filename to "%" to disable file logging.
 * 
 * @author Hazer
 * @last_modified 2025/04/13
 */

static constexpr int ERROR_WRONG_ORDER = 100;
static constexpr const char* DEFAULT_LOG_FILE = "runtime_log.txt";
enum errlevel {DEBUG, WARNING, ERROR};

class Log {
private:
    std::mutex log_lock;
    std::string file_path;
    bool enable_debug;
    Log();
    ~Log();
    Log(const Log&) = delete;
    Log& operator = (const Log&) = delete;
public:
    static Log& get_global_log();
    void change_file_path(const std::string& file_path);
    void start_debug();
    void end_debug();
    void err(const std::string& msg, errlevel level, int exitcode = 0);
};

}
