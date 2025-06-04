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
    explicit Node(const std::shared_ptr<DataType>& ptr = nullptr): data(ptr), ready(false) {}
    Node(const Node& node) = delete;
    Node& operator = (const Node& node) = delete;
    Node(Node&&) = delete;
    Node& operator = (Node&&) = delete;
    ~Node() {}
};

template <
    class DataType,
    class NodeType = Node<DataType>,
    class Container = std::deque<NodeType>,
    class Allocator = std::allocator<DataType>
>
class LockFreeQueue {
private:
    Container ring_buffer;
    Allocator element_allocator;
    std::atomic<size_t> head, tail;
    size_t capacity;
public:
    explicit LockFreeQueue(size_t length): 
    ring_buffer(length), element_allocator(), head(0), tail(0), capacity(length) {}
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator = (const LockFreeQueue&) = delete;
    LockFreeQueue(LockFreeQueue&&) = delete;
    LockFreeQueue& operator = (LockFreeQueue&&) = delete;
    ~LockFreeQueue() {}
};

}
