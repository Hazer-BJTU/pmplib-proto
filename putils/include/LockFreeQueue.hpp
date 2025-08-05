#pragma once

#include <exception>
#include <memory>
#include <atomic>
#include <thread>
#include <deque>

#include "GeneralException.h"

namespace putils {

template<class DataType>
struct DefaultLFQNode {
    using DataPtr = std::shared_ptr<DataType>;
    DataPtr data;
    std::atomic<bool> ready;
    DefaultLFQNode(const DataPtr& ptr = nullptr): data(ptr), ready(false) {}
    DefaultLFQNode(const DefaultLFQNode& node) = delete;
    DefaultLFQNode& operator = (const DefaultLFQNode& node) = delete;
    DefaultLFQNode(DefaultLFQNode&&) = delete;
    DefaultLFQNode& operator = (DefaultLFQNode&&) = delete;
    virtual ~DefaultLFQNode() {}
};

template <typename NodeType, typename DataType>
concept ValidNodeType = requires(NodeType node) {
    { node.data } -> std::same_as<std::shared_ptr<DataType>&>; //Nodes must contain 'data' field!
    { node.ready } -> std::same_as<std::atomic<bool>&>;        //Nodes must contain 'ready' field!
    requires std::is_default_constructible_v<NodeType>;        //Nodes should be default constructible.
};
/*If you want to customize and modify this class template, 
  it is recommended that you directly inherit from putils::DefaultLFQNode. */

template<typename Container, typename NodeType>
concept ValidContainer = requires(Container c, size_t size) {
    { c[size] } -> std::same_as<NodeType&>;                    //Container must support operator []! e.g., random access.
    requires std::is_default_constructible_v<Container>;       //Container should be default constructible.
    requires std::is_constructible_v<Container, size_t>;       //Container should support being constructed using size.
};

/**
 * @brief A lock-free queue implementation using a ring buffer.
 * 
 * @tparam DataType The type of data to be stored in the queue.
 * @tparam NodeType The node type used in the queue (must satisfy ValidNodeType concept).
 * @tparam Container The container type used as the underlying storage (must satisfy ValidContainer concept).
 * @tparam Allocator The allocator type for data allocation.
 * 
 * @section Design
 * This class implements a lock-free queue using a ring buffer design with the following characteristics:
 * 1. Thread-safe operations without locks (push/pop operations use atomic operations)
 * 2. Fixed-size circular buffer with power-of-two capacity for efficient modulo operations
 * 3. Producer-consumer pattern with separate head and tail pointers
 * 4. Node-based design with ready flags to coordinate between producers and consumers
 * 
 * The queue uses a "ready" flag mechanism to ensure proper synchronization between producers and consumers:
 * - Producers mark nodes as ready after filling them with data
 * - Consumers only process nodes marked as ready
 * - Each node can be in one of two states: ready for consumption or ready for production
 * 
 * @section Requirements
 * - NodeType must contain:
 *   - A 'data' member (std::shared_ptr<DataType>)
 *   - A 'ready' member (std::atomic<bool>)
 *   - Be default constructible
 * - Container must:
 *   - Support random access via operator[]
 *   - Be default constructible
 *   - Be constructible with a size parameter
 * 
 * @section Usage
 * Basic usage example:
 * @code
 * putils::LockFreeQueue<int> queue(1024);  // Create queue with capacity 1024
 * 
 * // Producer thread
 * auto data = std::make_shared<int>(42);
 * queue.try_push(data);  // Or queue.try_enqueue(42);
 * 
 * // Consumer thread
 * std::shared_ptr<int> received;
 * if (queue.try_pop(received)) {
 *     // Process received data
 * }
 * @endcode
 * 
 * @note
 * 1. Queue size must be a power of 2 (enforced in constructor)
 * 2. Minimum queue size is 4 (enforced in constructor)
 * 3. size() and empty() methods are approximate due to lock-free nature
 * 4. For custom node types, consider inheriting from DefaultLFQNode
 * 
 * @section Performance
 * - All operations are wait-free (no spinning)
 * - Uses memory ordering appropriate for each operation:
 *   - acquire for reads
 *   - release for writes
 *   - acq_rel for read-modify-write operations
 * - CAS (compare-and-swap) operations used for atomic updates
 * 
 * @author Hazer
 * @date 2025/6/4
 */

template <
    class DataType,
    class NodeType = DefaultLFQNode<DataType>,
    class Container = std::deque<NodeType>,
    class Allocator = std::allocator<DataType>                 //Only for data allocation.
> requires ValidNodeType<NodeType, DataType> && ValidContainer<Container, NodeType>
class LockFreeQueue {
private:
    using DataPtr = std::shared_ptr<DataType>;
    static constexpr size_t DEFAULT_BUFFER_LENGTH = 1024;
    static constexpr size_t CACHE_LINE_PADDING = 64;
    Container ring_buffer;
    Allocator element_allocator;
    alignas(CACHE_LINE_PADDING) std::atomic<size_t> head;
    alignas(CACHE_LINE_PADDING) std::atomic<size_t> tail;
    alignas(CACHE_LINE_PADDING) std::atomic<size_t> qlen;
    size_t capacity, mask;
public:
    LockFreeQueue(size_t length = DEFAULT_BUFFER_LENGTH): 
    ring_buffer(length), element_allocator(), head(0), tail(0), qlen(0), capacity(length) {
        if (capacity < 4) {
            throw PUTILS_GENERAL_EXCEPTION("Too short length for a queue!", "invalid argument");
        }
        mask = capacity - 1;
        if ((capacity & (capacity - 1)) != 0) {
            throw PUTILS_GENERAL_EXCEPTION("Queue length must be a power of 2!", "invalid argument");
        }
    }
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator = (const LockFreeQueue&) = delete;
    LockFreeQueue(LockFreeQueue&&) = delete;
    LockFreeQueue& operator = (LockFreeQueue&&) = delete;
    ~LockFreeQueue() {}
    template<typename Type>
    requires std::same_as<std::decay_t<Type>, DataPtr>
    bool try_push(Type&& data_ptr) noexcept {
        size_t current_tail, next_tail;
        do {
            current_tail = tail.load(std::memory_order_acquire);
            next_tail = (current_tail + 1) & mask;
            if (next_tail == head.load(std::memory_order_acquire)) {
                return false;
            }
            /* In a parallel environment, the head may have been modified, 
               but since the head in the ring buffer can only move in one direction, 
               we can still guarantee that the current_tail is safe at this point. */
        } while (
            //Ensure that consumers have completed node clearing.
            ring_buffer[current_tail].ready.load(std::memory_order_acquire) ||
            !tail.compare_exchange_weak( //Try to pre-occupy the node.
                current_tail, next_tail, //Move tail to next_tail.
                std::memory_order_acq_rel,
                std::memory_order_acquire
            )
            /* Note that if the evaluation of 'ready' fails, the short-circuiting property of || will prevent the subsequent CAS operation from executing. 
            If 'ready' evaluates to false but the node is modified by another producer thread between this evaluation and the CAS operation 
            (meaning the tail has already moved), then the subsequent CAS check will inevitably fail. At least when the ring buffer is sufficiently long, 
            it's unlikely for competing threads to be exactly one full cycle ahead of each other. */
        );
        //Complete node constructing.
        ring_buffer[current_tail].data = std::forward<Type>(data_ptr);
        ring_buffer[current_tail].ready.store(true, std::memory_order_release); //Ready for consume.
        qlen.fetch_add(1, std::memory_order_acq_rel);
        return true;
    }
    template<class... Args>
    bool try_enqueue(Args&&... args) noexcept {
        try {
            return try_push(std::allocate_shared<DataType>(element_allocator, std::forward<Args>(args)...));
        } catch (...) {
            //Just try again.
            return false;
        }
    }
    bool try_pop(DataPtr& data_ptr) noexcept {
        size_t current_head, next_head;
        do {
            current_head = head.load(std::memory_order_acquire);
            next_head = (current_head + 1) & mask;
            if (current_head == tail.load(std::memory_order_acquire)) {
                return false;
            }
            /* Same as what we have mentioned above, 
               we can guarantee that the current_head is safe at this point. */
        } while (
            //Ensure that producers have completed node constructing.
            !ring_buffer[current_head].ready.load(std::memory_order_acquire) || 
            !head.compare_exchange_weak( //Try to pre-release the node.
                current_head, next_head, //Move head to next_head.
                std::memory_order_acq_rel,
                std::memory_order_acquire
            )
            //See try_push().
        );
        //Complete node clearing.
        data_ptr = std::move(ring_buffer[current_head].data);
        ring_buffer[current_head].ready.store(false, std::memory_order_release); //Ready for produce.
        qlen.fetch_sub(1, std::memory_order_acq_rel);
        return true;
    }
    bool empty() const noexcept {
        /* return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed); */
        return qlen.load(std::memory_order_acquire) == 0;
    }
    size_t size() const noexcept {
        /* size_t current_head = head.load(std::memory_order_relaxed);
           size_t current_tail = tail.load(std::memory_order_relaxed);
           return (current_tail - current_head) & mask; */
        return qlen.load(std::memory_order_acquire);
    }
};

}
