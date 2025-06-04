#pragma once

#include <memory>
#include <atomic>
#include <thread>
#include <deque>

namespace putils {

template<class DataType>
struct Node {
    std::shared_ptr<DataType> data;
    std::atomic<bool> ready;
    explicit Node(const std::shared_ptr<DataType>& ptr): data(ptr), ready(false) {}
    Node(const Node& node) = delete;
    Node& operator = (const Node& node) = delete;
    Node(Node&&) = delete;
    Node& operator = (Node&&) = delete;
    ~Node() {}
}

template <
    class DataType,
    class NodeType = Node<DataType>
    class Container = std::deque<NodeType>,
    class Allocator = std::allocator<dataType>
>
class LockFreeQueue {
private:
    using dataPtr = std::shared_ptr<dataType>;
    struct Node {
        dataPtr data;
        std::atomic<bool> ready;
        Node(const dataPtr& ptr): data(ptr), ready(false) {}
        ~Node() {};
    };
    Container array;
    Allocator element_allocator;
public:

};

}
