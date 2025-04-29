#include "ArithmeticGraphNode.h"
#include <chrono>

using namespace rpc1k;

int main() {
    auto& taskhandler = ThreadPool::get_global_taskHandler();
    RealParser parser;
    auto node1 = std::make_shared<ConstantNode>("-9677821010012988e-8");
    auto node2 = std::make_shared<ConstantNode>("+9613210961378864e-8");
    
    std::cout << "Start construction!" << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<std::shared_ptr<GraphNode>> nodes;
    for (int i = 0; i < 200000; i++) {
        nodes.emplace_back(
            AddNode::construct_add_node_from_nodes(node1, node2)
        );
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "Time consumed: " << total_time << std::endl;

    std::cout << "Start calculate!" << std::endl;
    start_time = std::chrono::high_resolution_clock::now();
    node1->trigger();
    node2->trigger();
    taskhandler.wait_for_all_subgroups();
    end_time = std::chrono::high_resolution_clock::now();
    total_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "Time consumed: " << total_time << std::endl;
    
    for (int i = 0; i < 10; i++) {
        auto output = std::make_shared<ConstantNode>(nodes[i]);
        std::cout << parser(output) << std::endl;
    }
    return 0;
}