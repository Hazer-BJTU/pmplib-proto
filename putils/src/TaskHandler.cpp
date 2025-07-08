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
                            "(Worker): Task loss due to runtime errors.",
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

std::random_device ThreadPool::seed_generator;
size_t ThreadPool::num_executors = std::max<size_t>(1, std::thread::hardware_concurrency());
size_t ThreadPool::capacity_executors = 512;
size_t ThreadPool::num_workers_per_executor = 1;
size_t ThreadPool::num_distributors = std::max<size_t>(1, ThreadPool::num_executors / 4);
size_t ThreadPool::capacity_distributors = 2048;
size_t ThreadPool::num_workers_per_distributor = 1;
std::atomic<bool> ThreadPool::initialized = false;
std::mutex ThreadPool::setting_lock;

ThreadPool::ThreadPool(): executors(), distributors() {
    std::lock_guard<std::mutex> lock(ThreadPool::setting_lock);
    ThreadPool::initialized.store(true, std::memory_order_release);
    executors.reserve(ThreadPool::num_executors);
    for (int i = 0; i < ThreadPool::num_executors; i++) {
        executors.emplace_back(std::make_unique<TaskHandler>(
            ThreadPool::num_workers_per_executor,
            ThreadPool::capacity_executors
        ));
    }
    distributors.reserve(ThreadPool::num_distributors);
    for (int i = 0; i < ThreadPool::num_distributors; i++) {
        distributors.emplace_back(std::make_unique<TaskHandler>(
            ThreadPool::num_workers_per_distributor,
            ThreadPool::capacity_distributors
        ));
    }
}

ThreadPool::~ThreadPool() {}

ThreadPool::DistributionTask::DistributionTask(
    const TaskList& task_list, 
    size_t starting, 
    size_t ending,
    bool terminal
): task_list(), gtype(terminal ? ThreadPool::EXECUTE : ThreadPool::DISTRIBUTE) {
    if (ending == 0) {
        ending = task_list.size();
    }
    this->task_list.resize(ending - starting);
    std::copy(task_list.begin() + starting, task_list.begin() + ending, this->task_list.begin());
}

ThreadPool::DistributionTask::~DistributionTask() {}

void ThreadPool::DistributionTask::run() noexcept {
    auto& thread_pool = ThreadPool::get_global_threadpool();
    for (auto& task: task_list) {
        thread_pool.submit(task, gtype);
    }
    return;
}

bool ThreadPool::set_global_threadpool(
    size_t num_executors,
    size_t capacity_executors,
    size_t num_workers_per_executor,
    size_t num_distributors,
    size_t capacity_distributors,
    size_t num_workers_per_distributor
) noexcept {
    auto& logger = RuntimeLog::get_global_log();
    if (initialized.load(std::memory_order_acquire)) {
        logger.add("(ThreadPool): Settings cannot be modified after the instance has been initialized.");
        return false;
    }
    if (num_executors == 0 || num_workers_per_executor == 0 || num_distributors == 0 || num_workers_per_distributor == 0) {
        logger.add("(ThreadPool): All numeric arguments must be positive integers.");
        return false;
    }
    if (capacity_executors < 4 || (capacity_executors & (capacity_executors - 1)) != 0) {
        logger.add("(Executors): Argument 'capacity_executors' must be a power of 2 and at least 4.");
        return false;
    }
    if (capacity_distributors < 4 || (capacity_distributors & (capacity_distributors - 1)) != 0) {
        logger.add("(Distributors): Argument 'capacity_distributors' must be a power of 2 and at least 4.");
        return false;
    }

    ThreadPool::num_executors = num_executors;
    ThreadPool::capacity_executors = capacity_executors;
    ThreadPool::num_workers_per_executor = num_workers_per_executor;
    ThreadPool::num_distributors = num_distributors;
    ThreadPool::capacity_distributors = capacity_distributors;
    ThreadPool::num_workers_per_distributor = num_workers_per_distributor;
    
    size_t total_workers = ThreadPool::num_executors * ThreadPool::num_workers_per_executor + 
                           ThreadPool::num_distributors * ThreadPool::num_workers_per_distributor + 1;
    const size_t max_concurrency = std::thread::hardware_concurrency();
    float ratio = total_workers * 1.0f / max_concurrency;

    std::stringstream ss;
    ss << "(ThreadPool): potential workers (" << total_workers 
       << ") / maximum hardware concurrency (" << max_concurrency << ") = "
       << static_cast<size_t>(ratio * 100) << "%";
    logger.add(ss.str());
    
    if (ratio > 3) {
        logger.add("(ThreadPool): Thread usage exceeds 300% of hardware concurrency!", RuntimeLog::Level::WARN);
    }

    return true;
}

ThreadPool& ThreadPool::get_global_threadpool() noexcept {
    static ThreadPool threadpool_instance;
    return threadpool_instance;
}

void ThreadPool::submit(const TaskPtr& task, GroupType gtype) noexcept {
    thread_local std::mt19937 gen(ThreadPool::seed_generator());
    thread_local std::uniform_int_distribution<size_t> udist_executor(0, ThreadPool::num_executors - 1);
    thread_local std::uniform_int_distribution<size_t> udist_distributor(0, ThreadPool::num_distributors - 1);
    while(true) {
        auto& handler = gtype == EXECUTE ? executors[udist_executor(gen)] : distributors[udist_distributor(gen)];
        bool successful = handler->task_queue->try_push(task);
        if (successful) {
            handler->activate();
            break;
        } else {
            std::this_thread::yield();
        }
    }
    return;
}

void ThreadPool::shutdown() noexcept {
    for (auto& handler: executors) {
        handler->wait_all_done();
    }
    for (auto& handler: distributors) {
        handler->wait_all_done();
    }
    return;
}

void ThreadPool::task_distribute(const TaskList& task_list, bool as_single_task) noexcept {
    if (task_list.size() == 0) {
        return;
    }
    if (as_single_task) {
        auto task = std::make_shared<DistributionTask>(task_list);
        submit(task, ThreadPool::DISTRIBUTE);
    } else {
        size_t len = task_list.size();
        size_t block_size = std::max<size_t>(1, static_cast<size_t>(std::sqrt(len) + 0.5));
        TaskList dist_tasks;
        for (int i = 0; i < len; i += block_size) {
            dist_tasks.emplace_back(std::make_shared<DistributionTask>(task_list, i, std::min<size_t>(len, i + block_size), false));
        }
        task_distribute(dist_tasks, true);
    }
    return;
}

}
