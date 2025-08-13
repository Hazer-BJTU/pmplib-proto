#include "TaskHandler.h"

namespace putils {

Task::Task() {}

Task::~Task() {}

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
                        /* Attempt to fetch a task from the queue; 
                           if failed, use the empty() method to check whether 
                           the queue is truly empty rather than a spurious failure. */
                        try {
                            task_ptr->run();
                        } PUTILS_CATCH_LOG_GENERAL_MSG(
                            "(Worker): Task loss due to runtime errors.",
                            RuntimeLog::Level::WARN
                        )
                    } else if (task_queue->empty()) {
                        if (state.load(std::memory_order_acquire) == INACTIVE) {
                            /* Check the flag 'state' to see if an INACTIVE signal is received.
                               Here it shares the lock cv_lock with wait_all_done(),
                               and both condition variables cv_inactive and cv_all_done share cv_lock. */
                            std::unique_lock<std::mutex> lock(cv_lock);
                            active_workers.fetch_sub(1, std::memory_order_acq_rel);
                            cv_all_done.notify_all();
                            cv_inactive.wait(lock, [this]() -> bool { 
                                /* The condition variable cv_inactive blocks until state switches to ACTIVE or the flag quit becomes true.
                                   Waiting threads will be notified when activate() is called or during destruction. */
                                return state.load(std::memory_order_acquire) == ACTIVE || quit.load(std::memory_order_acquire);
                            });
                            if (quit.load(std::memory_order_acquire)) {
                                break;
                            }
                            active_workers.fetch_add(1, std::memory_order_acq_rel);
                        } else {
                            #ifdef PUTILS_THREADPOOL_WORKSTEALING_OPTIMIZATION
                                // Performs work stealing to fetch tasks from other queues.
                                task_ptr = ThreadPool::get_global_threadpool().work_stealing();
                                if (task_ptr) {
                                    try {
                                        task_ptr->run();
                                    } PUTILS_CATCH_LOG_GENERAL_MSG(
                                        "(Worker): Task loss due to runtime errors.",
                                        RuntimeLog::Level::WARN
                                    )
                                } else {
                                    // Steal failed, yield the time slice.
                                    std::this_thread::yield();
                                }
                            #else
                                std::this_thread::yield();
                            #endif
                        }
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

std::random_device ThreadPool::seed_generator;
size_t ThreadPool::num_executors = std::thread::hardware_concurrency();
size_t ThreadPool::executor_capacity = 1024;
size_t ThreadPool::num_workers_per_executor = 1;
std::atomic<bool> ThreadPool::initialized{false};
std::mutex ThreadPool::setting_lock;

ThreadPool::ThreadPool(): executors() {
    std::lock_guard<std::mutex> lock(ThreadPool::setting_lock);
    ThreadPool::initialized.store(true, std::memory_order_release);
    executors.reserve(ThreadPool::num_executors);
    for (int i = 0; i < ThreadPool::num_executors; i++) {
        executors.emplace_back(std::make_unique<TaskHandler>(
            ThreadPool::num_workers_per_executor,
            ThreadPool::executor_capacity
        ));
    }
}

ThreadPool::~ThreadPool() {}

size_t ThreadPool::get_executor_id() noexcept {
    thread_local std::mt19937 gen(seed_generator());
    thread_local std::uniform_int_distribution<size_t> udist(0, ThreadPool::num_executors - 1);
    size_t executor_id = udist(gen);
    return executor_id;
}

bool ThreadPool::set_global_threadpool(size_t num_executors, size_t executor_capacity, size_t num_workers_per_executor) noexcept {
    auto& logger = RuntimeLog::get_global_log();
    std::lock_guard<std::mutex> lock(ThreadPool::setting_lock);
    if (ThreadPool::initialized.load(std::memory_order_acquire)) {
        logger.add("(ThreadPool): Settings cannot be modified after the instance has been initialized.");
        return false;
    }
    if (num_executors == 0 || num_workers_per_executor == 0) {
        logger.add("(ThreadPool): All numeric arguments must be positive integers.");
        return false;
    }
    if (executor_capacity < 4 || (executor_capacity & (executor_capacity - 1)) != 0) {
        logger.add("(ThreadPool): Argument 'executor_capacity' must be a power of 2 and at least 4.");
        return false;
    }
    ThreadPool::num_executors = num_executors;
    ThreadPool::executor_capacity = executor_capacity;
    ThreadPool::num_workers_per_executor = num_workers_per_executor;
    size_t total_workers = ThreadPool::num_executors * ThreadPool::num_workers_per_executor + 1;
    const size_t max_concurrency = std::thread::hardware_concurrency();
    float ratio = total_workers * 1.0f / max_concurrency;
    
    std::stringstream ss;
    ss << "(ThreadPool): potential workers (" << total_workers 
       << ") / maximum hardware concurrency (" << max_concurrency << ") = "
       << static_cast<size_t>(ratio * 100) << "%";
    logger.add(ss.str());
    if (ratio > 3) {
        logger.add("(ThreadPool): Thread usage exceeds 300\% of hardware concurrency!", RuntimeLog::Level::WARN);
    }

    return true;
}

ThreadPool& ThreadPool::get_global_threadpool() noexcept {
    static ThreadPool threadpool_instance;
    return threadpool_instance;
}

void ThreadPool::submit(const TaskPtr& task) noexcept {
    while(true) {
        size_t executor_id = get_executor_id();
        bool successful = executors[executor_id]->task_queue->try_push(task);
        if (successful) {
            executors[executor_id]->activate();
            return;
        } else {
            std::this_thread::yield();
        }
    }
    return;
}

void ThreadPool::submit(const TaskList& task_list) noexcept {
    for (auto& task: task_list) {
        submit(task);
    }
    return;
}

ThreadPool::TaskPtr ThreadPool::work_stealing() noexcept {
    TaskPtr task;
    size_t starting_id = get_executor_id();
    for (
        size_t id = starting_id, next_id = (id + 1 == ThreadPool::num_executors) ? 0 : id + 1; 
        next_id != starting_id; 
        id = next_id, next_id = (id + 1 == ThreadPool::num_executors) ? 0 : id + 1
    ) {
        if (executors[id]->task_queue->try_pop(task)) {
            return task;
        }
    }
    return nullptr;
}

void ThreadPool::shutdown() noexcept {
    for (auto& executor: executors) {
        executor->wait_all_done();
    }
    return;
}

}
