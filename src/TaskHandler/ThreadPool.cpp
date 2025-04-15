#include "ThreadPool.h"

namespace rpc1k {

thread_local std::mt19937 Task::gen(DEFAULT_RANDOM_SEED);
thread_local std::uniform_int_distribution<int> Task::udist(UDIST_LOWER_BOUND, UDIST_UPPER_BOUND);

Task::Task() {
    task_idx = udist(gen);
}

Task::~Task() {}

void Task::run() {
    return;
}

int Task::get_task_idx() const {
    return task_idx;
}

bool ThreadPool::created = false;
int ThreadPool::total_workers = std::thread::hardware_concurrency();
int ThreadPool::num_groups = std::min(DEFAULT_QUEUE_NUM, ThreadPool::total_workers);
int ThreadPool::max_tasks = MAX_QUEUE_LENGTH;

ThreadPool::SubGroup::SubGroup(): end_flag(false), num_active_threads(0) {}

ThreadPool::SubGroup::~SubGroup() {}

bool ThreadPool::SubGroup::quit() {
    if (end_flag.load()) {
        return false;
    } else {
        end_flag.store(true);
        cv_not_empty.notify_all();
        return true;
    }
}

void ThreadPool::SubGroup::wait() {
    std::unique_lock<std::mutex> lock(q_lock);
    cv_all_done.wait(lock, [this]() -> bool {return num_active_threads.load() == 0 && task_q.empty();});
    return;
}

ThreadPool::ThreadPool() {
    created = true;
    //Initialize all the subgroups.
    for (int i = 0; i < num_groups; i++) {
        groups.emplace_back(std::make_unique<SubGroup>());
    }
    //Initialize all the workers.
    for (int i = 0; i < num_groups; i++) {
        int workers_per_group = total_workers / num_groups;
        if (i + 1 == num_groups) {
            workers_per_group += total_workers % num_groups;
        }
        for (int j = 0; j < workers_per_group; j++) {
            workers.emplace_back([this, &group = *groups[i]]() {
                while(true) {
                    std::shared_ptr<Task> task_ptr;
                    {
                        //In this block, we will try to grab a task from group.
                        std::unique_lock<std::mutex> lock(group.q_lock);
                        group.cv_not_empty.wait(lock, [this, &group]() -> bool {
                            return !group.task_q.empty() || group.end_flag.load();
                        });
                        if (!group.task_q.empty()) {
                            task_ptr = std::move(group.task_q.front());
                            group.task_q.pop();
                        } else {
                            //No task to do, the thread will quit.
                            return;
                        }
                    }
                    group.cv_not_full.notify_one();
                    //Now we have the task.
                    if (task_ptr) {
                        group.num_active_threads.fetch_add(1);
                        task_ptr->run();
                        group.num_active_threads.fetch_sub(1);
                    } else {
                        FREELOG("Unexpected modification of unfinished task!", errlevel::WARNING);
                    }
                    group.cv_all_done.notify_all();
                }
            });
        }
    }
}

ThreadPool::~ThreadPool() {
    wait_for_all_subgroups();
    for (int i = 0; i < num_groups; i++) {
        auto& group = *groups[i];
        group.quit();
    }
    for (auto& thread: workers) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

bool ThreadPool::set_global_taskHandler_config(int total_workers, int num_groups, int max_tasks) {
    if (created) {
        FREELOG("Configurations may not work because the instance has already been created!", errlevel::WARNING);
        return false;
    } else {
        if (total_workers < 1 || num_groups < 1 || max_tasks < 1) {
            FREELOG("Invalid arguments! All the arguments should be positive integers!", errlevel::WARNING);
            return false;
        }
        int max_concurrency = std::thread::hardware_concurrency();
        if (total_workers > max_concurrency) {
            FREELOG("The number of workers is higher than the maximum number of logical threads.", errlevel::DEBUG);
        }
        ThreadPool::total_workers = total_workers;
        if (num_groups > ThreadPool::total_workers) {
            FREELOG("Queue number is higher than the number of workers. Configuration not recommended!", errlevel::WARNING);
        }
        ThreadPool::num_groups = num_groups;
        ThreadPool::max_tasks = max_tasks;
        return true;
    }
}

ThreadPool& ThreadPool::get_global_taskHandler() {
    //Meyers' Singleton.
    static ThreadPool threadPool_instance;
    return threadPool_instance;
}

void ThreadPool::enqueue(const std::shared_ptr<Task>& task_ptr) {
    int task_idx = task_ptr->get_task_idx();
    auto& group = *groups[task_idx % num_groups];
    {
        //In this block, we will try to add a new task to the queue.
        std::unique_lock<std::mutex> lock(group.q_lock);
        group.cv_not_full.wait(lock, [this, &group]() -> bool {return (int)group.task_q.size() < max_tasks;});
        group.task_q.emplace(task_ptr);
    }
    group.cv_not_empty.notify_one();
    return;
}

void ThreadPool::wait_for_all_subgroups() {
    for (int i = 0; i < num_groups; i++) {
        auto& group = *groups[i];
        group.wait();
    }
    return;
}

}
