#pragma once

#include <thread>
#include <condition_variable>

#include "LockFreeQueue.hpp"
#include "GeneralException.h"
#include "RuntimeLog.h"

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
    void wait_all_done() noexcept;
    void activate() noexcept;
    void inactivate() noexcept;
};

class ThreadPool {
private:

public:

};

}
