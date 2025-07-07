#include "TaskHandler.h"

namespace putils {

TaskHandler::TaskHandler(size_t num_workers, size_t queue_capacity): 
workers(), active_workers(num_workers), cv_lock(), cv_inactive(), cv_all_done(), state(INACTIVE), quit(false) {
    try {
        task_queue = std::make_unique<LFQ>(queue_capacity);
        workers.reserve(num_workers);
        for (int i = 0; i < num_workers; i++) {
            workers.emplace_back([this]() {
                while(true) {
                    std::shared_ptr<Task> task_ptr;
                    if (task_queue->try_pop(task_ptr) && task_ptr) {
                        try {
                            task_ptr->run();
                        } PUTILS_CATCH_LOG_GENERAL_MSG(
                            "Task loss due to runtime errors.",
                            RuntimeLog::Level::WARN
                        )
                    } else if (task_queue->empty()) {
                        if (state.load(std::memory_order_acquire) == INACTIVE) {
                            std::unique_lock<std::mutex> lock(cv_lock);
                            active_workers.fetch_sub(1, std::memory_order_acq_rel);
                            cv_all_done.notify_all();
                            cv_inactive.wait(lock, [this]() -> bool { 
                                return state.load(std::memory_order_acquire) == ACTIVE || quit.load(std::memory_order_acquire);
                            });
                            if (quit.load(std::memory_order_acquire)) {
                                break;
                            }
                            active_workers.fetch_add(1, std::memory_order_acq_rel);
                        } else {
                            std::this_thread::yield();
                        }
                    } else {
                        std::this_thread::yield();
                    }
                }
            });
        }
    } PUTILS_CATCH_THROW_GENERAL
}

TaskHandler::~TaskHandler() {
    wait_all_done();
    quit.store(true, std::memory_order_release);
    cv_inactive.notify_all();
    for (auto& worker: workers) {
        worker.join();
    }
}

void TaskHandler::wait_all_done() noexcept {
    std::unique_lock<std::mutex> lock(cv_lock);
    inactivate();
    cv_all_done.wait(lock, [this]() -> bool { return active_workers.load(std::memory_order_acquire) == 0; });
    return;
}

void TaskHandler::activate() noexcept {
    state.store(ACTIVE, std::memory_order_release);
    cv_inactive.notify_all();
    return;
}

void TaskHandler::inactivate() noexcept {
    state.store(INACTIVE, std::memory_order_release);
    return;
}

}
