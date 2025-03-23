#include "ThreadPool.h"

namespace rpc1k {

void ThreadPool::enqueue(const std::weak_ptr<GraphNode>& node_ptr) {
    std::unique_lock<std::mutex> lock(q_lock);
    if (end_flag) {
        std::cerr << "[Warning]: Too late to add a new task." << std::endl;
        return;
    }
    space.wait(lock, [this]() -> bool {return this->q.size() < this->maxn;});
    q.emplace(node_ptr);
    content.notify_one();
    return;
}

std::weak_ptr<GraphNode> ThreadPool::grab() {
    std::unique_lock<std::mutex> lock1(q_lock);
    content.wait(lock1, [this]() -> bool {return !this->q.empty() || this->end_flag;});
    if (end_flag && this->q.empty()) {
        return std::weak_ptr<GraphNode>();
    } else {
        std::weak_ptr<GraphNode> node_ptr;
        do {
            node_ptr = q.front();
            q.pop();
            if (node_ptr.expired()) {
                std::cerr << "[Warning]: Unexpected task modification." << std::endl;
            }
        } while(node_ptr.expired() && !q.empty());
        space.notify_one();
        return node_ptr;
    }
}

ThreadPool::ThreadPool(size_t num_threads): maxn(MAXN_BLOCKING_QUEUE), end_flag(false), num_threads(num_threads) {
    for (size_t i = 0; i < num_threads; i++) {
        threads.emplace_back([this]() {
            while(true) {
                std::weak_ptr<GraphNode> node_ptr = this->grab();
                if (std::shared_ptr<GraphNode> snode_ptr = node_ptr.lock()) {
                    snode_ptr->run();
                } else {
                    break;
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    finish();
}

void ThreadPool::set(int idx) {
    std::unique_lock<std::mutex> lock(q_lock);
    if (idx < 0 || idx >= terminate_conditions.size()) {
        throw std::out_of_range("[Error]: Invalid condition index!");
    }
    terminate_conditions[idx] = true;
    for (auto cond: terminate_conditions) {
        if (!cond) {
            return;
        }
    }
    end_flag = true;
    all_completed.notify_all();
    return;
}

void ThreadPool::finish() {
    std::unique_lock<std::mutex> lock(q_lock);
    all_completed.wait(lock, [this]() -> bool {return this->end_flag;});
    content.notify_all();
    lock.unlock();
    for (auto& thread: threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    return;
}

int ThreadPool::add_terminal_task() {
    int idx = terminate_conditions.size();
    terminate_conditions.push_back(false);
    return idx;
}

}
