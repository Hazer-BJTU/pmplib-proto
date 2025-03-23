#include "CompGraphNodes.h"

namespace rpc1k {
    
GraphNode::GraphNode() {
    output = std::make_shared<BasicNum>(true);
}

GraphNode::~GraphNode() {}

void GraphNode::bind(const CompReign& cr) {
    //TO DO ...
}

void GraphNode::unbind() {
    //TO DO ...
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
    //TO DO ...
}

}
