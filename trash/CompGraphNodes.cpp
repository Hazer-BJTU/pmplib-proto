#include "CompGraphNodes.h"
#include "ThreadPool.h"
#include "CompReign.h"

namespace rpc1k {
    
GraphNode::GraphNode() {
    output = std::make_shared<BasicNum>(true);
}

GraphNode::~GraphNode() {}

void GraphNode::bind(const CompReign& cr) {
    task_handler = cr.get_task_handler();
    return;
}

void GraphNode::unbind() {
    task_handler = std::weak_ptr<ThreadPool>();
    return;
}

void GraphNode::set(int idx) {
    std::lock_guard<std::mutex> lock(conditon_lock);
    if (idx < 0 || idx >= conditions.size()) {
        throw std::out_of_range("[Error]: Invalid condition index!");
    }
    conditions[idx] = true;
    return;
}

void GraphNode::activate() {
    if (auto task_handler_shared = task_handler.lock()) {
        task_handler_shared->enqueue(weak_from_this());
    } else {
        throw std::logic_error("[Error]: Graph node without task handler!");
    }
    return;
}

int GraphNode::add_condition() {
    int idx = conditions.size();
    conditions.push_back(false);
    return idx;
}


//ConstantNode

ConstantNode::ConstantNode() {
    output = std::make_shared<BasicNum>();
}

ConstantNode::~ConstantNode() {}

ConstantNode::ConstantNode(std::string str) {
    output = std::make_shared<BasicNum>(str);
}

const std::shared_ptr<BasicNum>& ConstantNode::getNum() {
    return output;
}

void ConstantNode::run() {}

}
