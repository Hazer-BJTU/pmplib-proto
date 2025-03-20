#pragma once

#include "BasicNum.h"
#include "CompReign.h"
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>

namespace rpc1k {

class GraphNode {
public:
    std::vector<std::shared_ptr<GraphNode>> post, pre;
    std::vector<std::shared_ptr<BasicNum>> input;
    std::shared_ptr<ThreadPool> task_handler;
    std::shared_ptr<BasicNum> output;
    std::vector<int> condition_card;
    std::vector<bool> conditions;
    std::mutex conditon_lock;
    GraphNode();
    ~GraphNode();
    GraphNode(const GraphNode&) = delete;
    GraphNode& operator = (const GraphNode&) = delete;
    void bind(const CompReign& cr);
    void unbind();
    virtual void set(int idx) = 0;
    virtual void activate() = 0;
    virtual void run() = 0;
};

}
