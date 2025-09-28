#pragma once

#include <cmath>
#include <thread>
#include <condition_variable>

#include "LockFreeQueue.hpp"
#include "GeneralException.h"
#include "RuntimeLog.h"

namespace putils {

struct Task {
    Task();
    virtual ~Task() = 0;
    Task(const Task&) = default;
    Task& operator = (const Task&) = default;
    Task(Task&&) = default;
    Task& operator = (Task&&) = default;
    virtual void run() = 0;
    virtual std::string description() const noexcept = 0;
};

class ThreadPool;

/**
 * @class TaskHandler
 * @brief Internal worker management class for ThreadPool
 *
 * Manages a group of worker threads and their associated task queue.
 * Not intended for direct use - access through ThreadPool interface instead.
 *
 * Key Features:
 * - Manages worker thread lifecycle
 * - Implements task queue with lock-free operations
 * - Provides activation/inactivation control for workers
 * - Implements work stealing when queue is empty
 * - Synchronization via condition variables
 *
 * @warning This class is for internal ThreadPool use only
 * @see ThreadPool
 */

class TaskHandler {
private:
    using LFQ = LockFreeQueue<Task>;
    static constexpr bool ACTIVE = 1;
    static constexpr bool INACTIVE = 0;
    std::vector<std::thread> workers;
    std::atomic<int> active_workers;
    std::mutex cv_lock;
    std::condition_variable cv_inactive;
    std::condition_variable cv_all_done;
    std::unique_ptr<LFQ> task_queue;
    std::atomic<bool> state; //Non-volatile variable, no cache line padding is used here.
    std::atomic<bool> quit;
    friend ThreadPool;
public:
    TaskHandler(const size_t num_workers, const size_t queue_capacity, const size_t fail_threshold);
    ~TaskHandler();
    TaskHandler(const TaskHandler&) = delete;
    TaskHandler& operator = (const TaskHandler&) = delete;
    TaskHandler(TaskHandler&&) = delete;
    TaskHandler& operator = (TaskHandler&&) = delete;
    void wait_all_done() noexcept;
    void activate() noexcept;
    void inactivate() noexcept;
};

template<typename Lambda>
requires std::invocable<Lambda>
class InstantTask: public Task {
private:
    Lambda callback;
public:
    explicit InstantTask(Lambda&& target): callback(std::forward<Lambda>(target)) {}
    ~InstantTask() override {}
    InstantTask(const InstantTask&) = default;
    InstantTask& operator = (const InstantTask&) = default;
    InstantTask(InstantTask&&) = default;
    InstantTask& operator = (InstantTask&&) = default;
    void run() override {
        try {
            callback();
        } PUTILS_CATCH_THROW_GENERAL
    }
    std::string description() const noexcept override {
        std::stringstream ss;
        ss << "task[" << reinterpret_cast<uintptr_t>(this) << "]:unknown_lambda:no_info";
        return ss.str();
    }
};

template<typename Lambda>
requires std::invocable<Lambda>
std::shared_ptr<Task> wrap_task(Lambda&& target) noexcept {
    return std::make_shared<InstantTask<Lambda>>(std::forward<Lambda>(target));
}

/**
 * @class ThreadPool
 * @brief A high-performance thread pool implementation with work stealing capabilities
 *
 * This thread pool is designed for efficient parallel task execution with the following features:
 * - Lock-free task queue for minimal contention
 * - Work stealing between worker threads when queues are empty
 * - Dynamic activation/inactivation of worker threads
 * - Thread-safe task submission and synchronization
 * - Configurable number of executors and workers per executor
 *
 * Usage:
 * 1. Configure the thread pool using set_global_threadpool() before first use
 * 2. Get the singleton instance using get_global_threadpool()
 * 3. Submit tasks using submit() with either single TaskPtr or TaskList
 * 4. Use wrap_task() to easily create tasks from lambda expressions
 * 5. Call shutdown() to gracefully stop all workers (automatically called in destructor)
 *
 * Example:
 * @code
 * // Configure (optional, defaults to hardware concurrency)
 * ThreadPool::set_global_threadpool(4, 1024, 2);
 * 
 * // Get instance
 * auto& pool = ThreadPool::get_global_threadpool();
 * 
 * // Submit tasks
 * std::latch latch{2};
 * pool.submit(wrap_task([&]{ latch.count_down(); }));
 * pool.submit(wrap_task([&]{ latch.count_down(); }));
 * latch.wait();
 * @endcode
 *
 * Implementation Details:
 * - Uses TaskHandler as internal worker management class
 * - Each executor has its own task queue and worker threads
 * - Workers automatically steal work from other queues when idle
 * - Tasks are submitted to random queues to balance load
 * - Provides wait_all_done() for synchronization
 *
 * @note The thread pool is implemented as a singleton. Use get_global_threadpool() to access it.
 * @warning Changing configuration after initialization has no effect.
 * 
 * @author Hazer
 * @date 2025/7/10
 */

class ThreadPool {
public:
    using Partition = std::vector<std::unique_ptr<TaskHandler>>;
    using TaskList = std::vector<std::shared_ptr<Task>>;
    using TaskPtr = std::shared_ptr<Task>;
private:
    static std::random_device seed_generator;
    static size_t num_executors;
    static size_t executor_capacity;
    static size_t num_workers_per_executor;
    static size_t fail_block_threshold;
    static std::atomic<bool> initialized;
    static std::mutex setting_lock;
    Partition executors;
    ThreadPool();
    ~ThreadPool();
    size_t get_executor_id() noexcept;
public:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator = (const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator = (ThreadPool&) = delete;
    static bool set_global_threadpool(
        size_t num_executors = ThreadPool::num_executors,
        size_t executor_capacity = ThreadPool::executor_capacity,
        size_t num_workers_per_executor = ThreadPool::num_workers_per_executor,
        size_t fial_block_threshold = ThreadPool::fail_block_threshold
    ) noexcept;
    static ThreadPool& get_global_threadpool() noexcept;
    void submit(const TaskPtr& task) noexcept;
    void submit(const TaskList& task_list) noexcept;
    TaskPtr work_stealing() noexcept;
    void shutdown() noexcept;
};

}
