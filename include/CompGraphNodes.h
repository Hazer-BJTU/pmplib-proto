#pragma once

#include "BasicNum.h"
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>

namespace rpc1k {

class CompReign;
class ThreadPool;

class GraphNode {
public:
    std::vector<std::weak_ptr<GraphNode>> post, pre;
    std::vector<std::weak_ptr<BasicNum>> input;
    std::shared_ptr<BasicNum> output;
    std::weak_ptr<ThreadPool> task_handler;
    std::vector<int> condition_card;
    std::vector<bool> conditions;
    std::mutex conditon_lock;
    GraphNode();
    ~GraphNode();
    GraphNode(const GraphNode&) = delete;
    GraphNode& operator = (const GraphNode&) = delete;
    void bind(const CompReign& cr);
    void unbind();
    void set(int idx);
    void activate();
    virtual void run() = 0;
};

}
