#pragma once

#include <cmath>
#include <thread>
#include <condition_variable>

#include "LockFreeQueue.hpp"
#include "GeneralException.h"
#include "RuntimeLog.h"
#include "task.hpp"

namespace putils {

class Task;
class ThreadPool;

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
    std::atomic<bool> state;
    std::atomic<bool> quit;
    friend ThreadPool;
public:
    TaskHandler(size_t num_workers, size_t queue_capacity);
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
    ~InstantTask() {}
    InstantTask(const InstantTask&) = default;
    InstantTask& operator = (const InstantTask&) = default;
    InstantTask(InstantTask&&) = default;
    InstantTask& operator = (InstantTask&&) = default;
    void run() override {
        try {
            callback();
        } PUTILS_CATCH_THROW_GENERAL
    }
};

template<typename Lambda>
requires std::invocable<Lambda>
std::shared_ptr<Task> wrap_task(Lambda&& target) noexcept {
    return std::make_shared<InstantTask<Lambda>>(std::forward<Lambda>(target));
}

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
    static std::atomic<bool> initialized;
    static std::mutex setting_lock;
    Partition executors;
    ThreadPool();
    ~ThreadPool();
    TaskHandler& get_executor() noexcept;
public:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator = (const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator = (ThreadPool&) = delete;
    static bool set_global_threadpool(
        size_t num_executors = ThreadPool::num_executors,
        size_t executor_capacity = ThreadPool::executor_capacity,
        size_t num_workers_per_executor = ThreadPool::num_workers_per_executor
    ) noexcept;
    static ThreadPool& get_global_threadpool() noexcept;
    void submit(const TaskPtr& task) noexcept;
    void submit(const TaskList& task_list) noexcept;
    TaskPtr work_stealing() noexcept;
    void shutdown() noexcept;
};

}
