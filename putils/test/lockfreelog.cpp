#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include "LockFreeQueue.hpp"

class LockFreeLogger {
public:
    // 获取日志实例(单例模式)
    static LockFreeLogger& instance() {
        static LockFreeLogger logger;
        return logger;
    }

    // 初始化日志系统
    void init(const std::string& filename, 
              std::chrono::milliseconds flush_interval = std::chrono::seconds(5),
              size_t queue_size = 1024) {
        if (initialized_.exchange(true)) {
            return;
        }
        
        filename_ = filename;
        flush_interval_ = flush_interval;
        exit_flag_.store(false);
        
        // 初始化无锁队列
        log_queue_ = std::make_unique<putils::LockFreeQueue<std::string>>(queue_size);
        
        // 启动刷新线程
        flush_thread_ = std::thread(&LockFreeLogger::flush_thread_func, this);
        
        // 注册退出处理函数
        register_exit_handlers();
    }

    // 写入日志
    void log(const std::string& message) {
        if (!initialized_.load()) {
            return;
        }
        
        // 尝试入队，如果队列满则丢弃日志
        if (!log_queue_->try_enqueue(message)) {
            // 可以在这里添加队列满时的处理逻辑
            std::cerr << "Log queue is full, message dropped: " << message << std::endl;
        }
    }

    // 关闭日志系统
    void shutdown() {
        if (!initialized_.exchange(false)) {
            return;
        }
        
        exit_flag_.store(true);
        
        if (flush_thread_.joinable()) {
            flush_thread_.join();
        }
        
        // 确保所有日志都已写入
        flush_remaining_logs();
    }

    // 禁止复制和移动
    LockFreeLogger(const LockFreeLogger&) = delete;
    LockFreeLogger& operator=(const LockFreeLogger&) = delete;
    LockFreeLogger(LockFreeLogger&&) = delete;
    LockFreeLogger& operator=(LockFreeLogger&&) = delete;

private:
    LockFreeLogger() = default;
    ~LockFreeLogger() {
        shutdown();
    }

    // 刷新线程函数
    void flush_thread_func() {
        std::ofstream file;
        file.open(filename_, std::ios::app);
        
        if (!file.is_open()) {
            std::cerr << "Failed to open log file: " << filename_ << std::endl;
            return;
        }
        
        while (!exit_flag_.load()) {
            // 定期刷新
            auto start = std::chrono::steady_clock::now();
            
            // 处理队列中的日志
            process_queue(file);
            
            // 计算剩余等待时间
            auto elapsed = std::chrono::steady_clock::now() - start;
            auto remaining_wait = flush_interval_ - elapsed;
            
            if (remaining_wait > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(remaining_wait);
            }
        }
        
        // 退出前处理剩余日志
        process_queue(file);
        file.close();
    }

    // 处理队列中的日志
    void process_queue(std::ofstream& file) {
        std::shared_ptr<std::string> log_entry;
        while (log_queue_->try_pop(log_entry)) {
            file << *log_entry << "\n";
        }
        file.flush();
    }

    // 刷新剩余日志
    void flush_remaining_logs() {
        std::ofstream file(filename_, std::ios::app);
        if (!file.is_open()) {
            return;
        }
        
        std::shared_ptr<std::string> log_entry;
        while (log_queue_->try_pop(log_entry)) {
            file << *log_entry << "\n";
        }
        file.flush();
        file.close();
    }

    // 注册退出处理函数
    void register_exit_handlers() {
        // 正常退出处理
        std::atexit([]() {
            LockFreeLogger::instance().shutdown();
        });
        
        // 异常终止处理
        std::set_terminate([]() {
            LockFreeLogger::instance().shutdown();
            std::abort();
        });
        
        // 信号处理
        std::signal(SIGINT, [](int) {
            LockFreeLogger::instance().shutdown();
            std::exit(SIGINT);
        });
        std::signal(SIGTERM, [](int) {
            LockFreeLogger::instance().shutdown();
            std::exit(SIGTERM);
        });
    }

private:
    std::string filename_;
    std::unique_ptr<putils::LockFreeQueue<std::string>> log_queue_;
    std::thread flush_thread_;
    std::chrono::milliseconds flush_interval_;
    std::atomic<bool> exit_flag_{false};
    std::atomic<bool> initialized_{false};
};

// 使用示例
int main() {
    // 初始化日志系统
    LockFreeLogger::instance().init("app.log", std::chrono::seconds(3), 1024);
    
    // 多线程写入日志
    auto log_task = []() {
        for (int i = 0; i < 100; ++i) {
            LockFreeLogger::instance().log("Thread " + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + 
                                          " - Message " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    
    std::thread t1(log_task);
    std::thread t2(log_task);
    std::thread t3(log_task);
    
    t1.join();
    t2.join();
    t3.join();
    
    // 正常退出时会自动刷新
    return 0;
}