#pragma once

#include "Log.h"
#include <atomic>
#include <random>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <algorithm>
#include <condition_variable>

#define SET_PARALLEL(total_workers, num_groups, max_tasks) ::rpc1k::ThreadPool::set_global_taskHandler_config(total_workers, num_groups, max_tasks)

namespace rpc1k {

static constexpr int UDIST_LOWER_BOUND = 0;
static constexpr int UDIST_UPPER_BOUND = 255;

/*
 * This is a thread pool task interface (base class) with a thread_local random number generator,
 * using random_device to obtain true random seeds. 
 * 
 * Note: Any instantiation of a concrete task subclass must occur AFTER the instantiation 
 * of the global thread pool. This is because we rely on a static variable `num_groups` 
 * to determine the range of random task IDs.
 * 
 * To implement a task subclass, you MUST override the following function:
 *     virtual void run();
 * 
 * @Author: Hazer
 * @Last modified: 2025/4/15
 */
class Task {
private:
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen;
    static thread_local std::uniform_int_distribution<int> udist;
    int task_idx;
public:
    Task();
    ~Task();
    Task(const Task&) = delete;
    Task& operator = (const Task&) = delete;
    virtual void run();
    int get_task_idx() const;
};

static constexpr int MAX_QUEUE_LENGTH = 512;

/*​
 * The global thread pool consists of multiple independent SubGroups and 
 * centrally manages all workers, implemented using the Meyers' Singleton pattern.
 * 
 * Important usage requirements:
 * - Users must fully understand task dependencies
 * - Proper synchronization and destruction timing is critical
 * 
 * Usage recommendations:
 * - Not recommended for direct use by end users
 * - Users should utilize the proper interfaces instead
 *   of directly calling thread pool APIs
 * 
 * Implementation note:
 * - The logging class (Log.h) used is thread-safe
 */
class ThreadPool {
private:
    static bool created;
    static int total_workers;
    static int num_groups;
    static int max_tasks;
    /*
     * SubGroup is the fundamental building block of a thread pool, containing:
     * - An independent blocking task queue
     * - A dedicated lock for controlling access
     * 
     * Atomic variables:
     * - end_flag: Termination indicator (typically only used during thread pool destruction)
     * - num_active_threads: Count of currently active worker threads
     * 
     * Condition variables:
     * - cv_not_empty & cv_not_full: Implement producer-consumer pattern
     * - cv_all_done: Used for task synchronization
     * 
     * Key design principles:
     * - Complete independence between units:
     *   * No interaction via static/atomic variables
     *   * No shared condition variables or locks
     * - Task-to-unit assignment is determined at task creation time
     */
    class SubGroup {
    public:
        std::atomic<bool> end_flag;                //Exit flag for all workers.
        std::atomic<int> num_active_threads;       //Number of active workers.
        std::queue<std::shared_ptr<Task>> task_q;  //Task queue, protected by q_lock.
        std::mutex q_lock;                         //Mutually exclusive lock.
        std::condition_variable cv_not_empty;      //Condition variable for Producer-Consumer model.
        std::condition_variable cv_not_full;       //Condition variable for Producer-Consumer model.
        std::condition_variable cv_all_done;       //Condition variable for task synchronization.
        SubGroup();
        ~SubGroup();
        SubGroup(const SubGroup&) = delete;
        SubGroup& operator = (const SubGroup&) = delete;
        bool quit();
        void wait();
    };
    std::vector<std::unique_ptr<SubGroup>> groups;
    std::vector<std::thread> workers;
    ThreadPool();
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator = (const ThreadPool&) = delete;
public:
    static bool instance_created();
    static int get_num_groups();
    static bool set_global_taskHandler_config(int total_workers, int num_groups, int max_tasks);
    static ThreadPool& get_global_taskHandler();
    void enqueue(const std::shared_ptr<Task>& task_ptr);
    void wait_for_all_subgroups();
};

}
