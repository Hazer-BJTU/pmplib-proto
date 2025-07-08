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

class ThreadPool {
public:
    using Partition = std::vector<std::unique_ptr<TaskHandler>>;
    using TaskList = std::vector<std::shared_ptr<Task>>;
    using TaskPtr = std::shared_ptr<Task>;
    using GroupType = int;
private:
    static std::random_device seed_generator;
    static size_t num_executors;
    static size_t capacity_executors;
    static size_t num_workers_per_executor;
    static size_t num_distributors;
    static size_t capacity_distributors;
    static size_t num_workers_per_distributor;
    static std::atomic<bool> initialized;
    static std::mutex setting_lock;
    Partition executors;
    Partition distributors;
    ThreadPool();
    ~ThreadPool();
private:
    class DistributionTask: public Task {
    private:
        TaskList task_list;
        GroupType gtype;
    public:
        explicit DistributionTask(
            const TaskList& task_list, 
            size_t starting = 0, 
            size_t ending = 0,
            bool terminal = true
        );
        ~DistributionTask();
        DistributionTask(const DistributionTask&) = default;
        DistributionTask& operator = (const DistributionTask&) = default;
        DistributionTask(DistributionTask&&) = default;
        DistributionTask& operator = (DistributionTask&&) = default;
        void run() noexcept override;
    };
public:
    static constexpr GroupType EXECUTE = 0;
    static constexpr GroupType DISTRIBUTE = 1;
    static bool set_global_threadpool(
        size_t num_executors = ThreadPool::num_executors,
        size_t capacity_executors = ThreadPool::capacity_executors,
        size_t num_workers_per_executor = ThreadPool::num_workers_per_executor,
        size_t num_distributors = ThreadPool::num_distributors,
        size_t capacity_distributor = ThreadPool::capacity_distributors,
        size_t num_workers_per_distributor = ThreadPool::num_workers_per_distributor
    ) noexcept;
    static ThreadPool& get_global_threadpool() noexcept;
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator = (const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator = (ThreadPool&) = delete;
    void submit(const TaskPtr& task, GroupType gtype) noexcept;
    void shutdown() noexcept;
    template<typename Lambda>
    requires std::invocable<Lambda>
    void submit(Lambda&& target, GroupType gtype) noexcept {
        auto task = std::make_shared<InstantTask<Lambda>>(std::forward<Lambda>(target));
        submit(task, gtype);
        return;
    }
    void task_distribute(const TaskList& task_list, bool as_single_task = false) noexcept;
};

template<typename Lambda>
requires std::invocable<Lambda>
static ThreadPool::TaskPtr wrap_task(Lambda&& target) noexcept {
    return std::make_shared<InstantTask<Lambda>>(std::forward<Lambda>(target));
}

}
