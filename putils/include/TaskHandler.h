#pragma once

#include <thread>
#include <condition_variable>

#include "LockFreeQueue.hpp"
#include "GeneralException.h"
#include "RuntimeLog.h"
#include "task.hpp"

namespace putils {

class Task;

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
class InstantTask: public Task {
private:
    Lambda callback;
public:
    InstantTask(Lambda&& target): callback(std::forward<Lambda>(target)) {}
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
private:

public:

};

}
