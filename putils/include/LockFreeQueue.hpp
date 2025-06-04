#pragma once

#include <exception>
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
    using DataPtr = std::shared_ptr<DataType>;
    Container ring_buffer;
    Allocator element_allocator;
    std::atomic<size_t> head, tail;
    size_t capacity, mask;
public:
    explicit LockFreeQueue(size_t length): 
    ring_buffer(length), element_allocator(), head(0), tail(0), capacity(length) {
        if (capacity < 4) {
            throw std::invalid_argument("Too short length for a queue!");
        }
        mask = capacity - 1;
        if ((capacity & (capacity - 1)) != 0) {
            throw std::invalid_argument("Queue length must be a power of 2!");
        }
    }
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator = (const LockFreeQueue&) = delete;
    LockFreeQueue(LockFreeQueue&&) = delete;
    LockFreeQueue& operator = (LockFreeQueue&&) = delete;
    ~LockFreeQueue() {}
    bool try_push(const DataPtr& data_ptr) noexcept {
        bool is_occupied;
        size_t current_tail, next_tail;
        do {
            current_tail = tail.load(std::memory_order_acquire);
            next_tail = (current_tail + 1) & mask;
            if (next_tail == head.load(std::memory_order_acquire)) {
                return false;
            }
            is_occupied = ring_buffer[current_tail].ready.load(std::memory_order_acquire);
        } while (
            is_occupied || //Ensure that consumers have completed node clearing.
            !tail.compare_exchange_weak( //Try to pre-occupy the node.
                current_tail, next_tail, //Mode tail to next_tail.
                std::memory_order_acq_rel,
                std::memory_order_acquire
            )
        );
        //Complete node constructing.
        ring_buffer[current_tail].data = data_ptr;
        ring_buffer[current_tail].ready.store(true, std::memory_order_release); //Ready for consume.
        return true;
    }
    template<class... Args>
    bool try_enqueue(Args&&... args) noexcept {
        DataPtr new_data_ptr;
        try {
            new_data_ptr = std::allocate_shared<DataType>(element_allocator, std::forward<Args>(args)...);
        } catch (...) {
            return false;
        }
        return try_push(new_data_ptr);
    }
    bool try_pop(DataPtr& data_ptr) noexcept {
        bool is_ready;
        size_t current_head, next_head;
        do {
            current_head = head.load(std::memory_order_acquire);
            next_head = (current_head + 1) & mask;
            if (current_head == tail.load(std::memory_order_acquire)) {
                return false;
            }
            is_ready = ring_buffer[current_head].ready.load(std::memory_order_acquire);
        } while (
            !is_ready || //Ensure that producers have completed node constructing.
            !head.compare_exchange_weak( //Try to pre-release the node.
                current_head, next_head, //Move head to next_head.
                std::memory_order_acq_rel,
                std::memory_order_acquire
            )
        );
        //Complete node clearing.
        data_ptr = std::move(ring_buffer[current_head].data);
        ring_buffer[current_head].ready.store(false, std::memory_order_release); //Ready for produce.
        return true;
    }
    bool empty() const noexcept {
        //Not precise!
        return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
    }
    size_t size() const noexcept {
        //Not precise!
        size_t current_head = head.load(std::memory_order_relaxed);
        size_t current_tail = tail.load(std::memory_order_relaxed);
        return (current_tail - current_head) & mask;
    }
};

}
