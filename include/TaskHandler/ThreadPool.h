#pragma once

#include <random>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace rpc1k {

static constexpr int DEFAULT_RANDOM_SEED = 42;
static constexpr int UDIST_LOWER_BOUND = 0;
static constexpr int UDIST_UPPER_BOUND = 256;

class Task {
private:
    static std::mt19937 gen;
    static std::uniform_int_distribution<int> udist;
    int task_idx;
public:
    Task();
    ~Task();
    virtual void run();
};

static constexpr int MAX_QUEUE_LENGTH = 256;

class ThreadPool {
private:
    static bool created;
    static int total_workers;
    static int num_queues;
    static int maxn;
    bool end_flag;
    std::vector<std::queue<std::weak_ptr<Task>>> task_qs;
    std::vector<std::mutex> q_locks;
    std::vector<std::condition_variable> spaces;
    std::vector<std::condition_variable> contents;
    std::vector<std::vector<std::thread>> worker_groups;
    ThreadPool();
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator = (const ThreadPool&) = delete;
public:
    static bool setGlobalTaskHandlerConfig(int total_workers, int num_queues, int maxn);
    static ThreadPool& getGlobalTaskHandler();
};

}
