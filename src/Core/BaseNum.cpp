#include "BaseNum.h"

namespace rpc1k {

BaseNum::BaseNum(): sign(1) {
    auto& allocator = SegmentAllocator::get_global_allocator();
    allocator.assign(data);
}

BaseNum::~BaseNum() {
    auto& allocator = SegmentAllocator::get_global_allocator();
    allocator.free(data);
}

BaseNum::BaseNum(const BaseNum& num) {
    auto& allocator = SegmentAllocator::get_global_allocator();
    allocator.assign(data);
    sign = num.sign;
    memcpy(data, num.data, LENGTH * sizeof(int64));
}

BaseNum& BaseNum::operator = (const BaseNum& num) {
    if (this == &num) {
        return *this;
    }
    auto& allocator = SegmentAllocator::get_global_allocator();
    allocator.exchange(data);
    memcpy(data, num.data, LENGTH * sizeof(int64));
    sign = num.sign;
    return *this;
}

const int64& BaseNum::operator [] (int idx) const {
    if (idx < 0 || idx >= LENGTH) {
        FREELOG("Index access out of range!", errlevel::WARNING);
        return data[0];
    }
    return data[idx];
}

bool BaseNum::get_sign() const {
    return sign;
}

GraphNode::GraphNode(): inp_dept_counter(0), red_dept_counter(0), reduce(nullptr) {
    out_domain = std::make_shared<BaseNum>();
}

GraphNode::~GraphNode() {}

void GraphNode::inp_count_down() {
    if (inp_dept_counter.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        //Only one thread will enter this block, so no synchronization is needed.
        auto& task_handler = ThreadPool::get_global_taskHandler();
        for (auto& mission: workload) {
            task_handler.enqueue(std::move(mission));
            //red_count_down() will be called in mission.
        }
        workload.clear();
    }
    return;
}

void GraphNode::red_count_down() {
    if (red_dept_counter.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        //Only one thread will enter this block, so no synchronization is needed.
        if (reduce) {
            //There exists a reduce job.
            auto& task_handler = ThreadPool::get_global_taskHandler();
            task_handler.enqueue(std::move(reduce));
            //notify_successors() will be called in reduce.
        } else {
            //There doesn't exist a reduce job.
            notify_successors();
        }
    }
    return;
}

void GraphNode::notify_successors() {
    for (auto& wptr: successors) {
        auto ptr = wptr.lock();
        if (ptr) {
            ptr->inp_count_down();
        } else {
            AUTOLOG("Inconsistencies found in the computation DAG!", errlevel::ERROR, ERROR_COMPUTE_HALT);
        }
    }
    for (auto& ptr: input_domains) {
        //The input domains are no longer useful.
        ptr.reset();
    }
    input_domains.clear();
    return;
}

bool GraphNode::is_triggerable() {
    return false;
}

TriggerableNode::TriggerableNode() {}

TriggerableNode::~TriggerableNode() {}

bool TriggerableNode::is_triggerable() {
    return true;
}

void TriggerableNode::trigger() {
    notify_successors();
    return;
}

}
