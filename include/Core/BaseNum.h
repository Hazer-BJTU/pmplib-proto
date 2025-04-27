#pragma once

#include "Log.h"
#include "Prememory.h"
#include "ThreadPool.h"
#include <cstdlib>
#include <cstring>

namespace rpc1k {

using int64 = unsigned long long;

inline constexpr size_t LENGTH = SEGMENT_SIZE / sizeof(int64);
inline constexpr int BASE = 1e8;
inline constexpr int LGBASE = 8;
inline constexpr int ZERO = LENGTH / 2 - 1;

class RealParser;

class BaseNum {
private:
    bool sign;
    int64* data;
    friend RealParser;
public:
    BaseNum();
    ~BaseNum();
    BaseNum(const BaseNum& num);
    BaseNum& operator = (const BaseNum& num);
    const int64& operator [] (int idx) const;
    bool get_sign() const;
};

/**
 * @brief Base class for computation graph nodes implementing lock-free task dependency
 *        through atomic counters.
 * 
 * The computation graph processes dependencies without locks using simple atomic counters.
 * Each node can have multiple parallelizable task workloads. The last thread to complete
 * its workload handles the reduction task and notifies all successor nodes.
 * 
 * Key characteristics:
 * - Manages its own output domain (out_domain) using shared_ptr
 * - References predecessor outputs as input domains via shared_ptr
 * - Supports multiple input domains but has exactly one output domain
 * - Uses two atomic counters:
 *   * inp_dept_counter: Tracks input dependencies
 *   * red_dept_counter: Tracks reduction dependencies
 * 
 * Derived class TriggerableNode:
 * - Can be manually activated (triggered)
 * - Typically used for constant nodes in the DAG
 * - Overrides is_triggerable() and adds trigger() method
 * 
 * The node lifecycle involves:
 * 1. Waiting for input dependencies (inp_dept_counter)
 * 2. Executing parallel workloads when ready
 * 3. Performing reduction when last workload completes
 * 4. Notifying successor nodes
 * 
 * Thread safety is maintained through:
 * - Atomic operations with appropriate memory ordering (memory_order_acq_rel)
 * - Single-threaded reduction guarantee (only last thread executes reduce())
 * - Shared_ptr/weak_ptr for safe memory management
 * @author Hazer
 * @date 2025/4/27
 */
class GraphNode {
private:
    //Data field.
    std::vector<std::weak_ptr<GraphNode>> successors, precursors;
    std::shared_ptr<BaseNum> out_domain;
    std::vector<std::shared_ptr<BaseNum>> input_domains;
    std::atomic<int> inp_dept_counter;
    //Work field.
    std::vector<std::shared_ptr<Task>> workload;
    std::atomic<int> red_dept_counter;
protected:
    virtual void reduce();
    void inp_count_down();
    void red_count_down();
    void notify_successors();
public:
    GraphNode();
    ~GraphNode();
    GraphNode(const GraphNode&) = delete;
    GraphNode& operator = (const GraphNode&) = delete;
    virtual bool is_triggerable();
};

class TriggerableNode: public GraphNode {
public:
    TriggerableNode();
    ~TriggerableNode();
    TriggerableNode(const TriggerableNode&) = delete;
    TriggerableNode& operator = (const TriggerableNode&) = delete;
    bool is_triggerable() override;
    void trigger();
};

}
