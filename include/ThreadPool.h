#pragma once

#include "CompGraphNodes.h"
#include <condition_variable>
#include <iostream>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <exception>

namespace rpc1k {

static constexpr size_t MAXN_BLOCKING_QUEUE = 256;

class ThreadPool {
public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator = (const ThreadPool&) = delete;
    //We need to realize a blocking queue.
    size_t maxn;
    std::queue<std::weak_ptr<GraphNode>> q;
    std::mutex q_lock;
    std::condition_variable space;
    std::condition_variable content;
    void enqueue(const std::weak_ptr<GraphNode>& node_ptr);
    std::weak_ptr<GraphNode> grab();
    //Other members
    bool end_flag;
    size_t num_threads;
    std::vector<std::thread> threads;
    std::vector<bool> terminate_conditions;
    std::mutex conditions_lock;
    std::condition_variable all_completed;
    void set(int idx);
    void finish();
};

}
