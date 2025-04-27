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

class GraphNode {
private:
    //Data field.
    std::vector<std::weak_ptr<GraphNode>> successors, precursors;
    std::shared_ptr<BaseNum> out_domain;
    std::vector<std::shared_ptr<BaseNum>> input_domains;
    std::atomic<int> inp_dept_counter;
    //Work field.
    std::vector<std::shared_ptr<Task>> workload;
    std::shared_ptr<Task> reduce;
    std::atomic<int> red_dept_counter;
protected:
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
