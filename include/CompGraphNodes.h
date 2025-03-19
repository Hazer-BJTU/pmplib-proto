#pragma once

#include "BasicNum.h"
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>

namespace rpc1k {

class GraphNode {
public:
    std::vector<std::shared_ptr<GraphNode>> post, pre;
    std::vector<std::shared_ptr<BasicNum>> input, output;
    std::vector<bool> conditions;
    std::mutex conditon_lock;
    GraphNode();
    ~GraphNode();
    GraphNode(const GraphNode&) = delete;
    GraphNode& operator = (const GraphNode&) = delete;
};

}
