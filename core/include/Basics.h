#pragma once

#include <list>
#include <latch>
#include <string.h>

#include "FiniteStateMachine.hpp"
#include "MemoryAllocator.h"
#include "GlobalConfig.h"
#include "RuntimeLog.h"
#include "TaskHandler.h"

namespace mpengine {

struct BasicIntegerType {
    using ElementType = uint64_t;
    using BlockHandle = putils::MemoryPool::BlockHandle;
    static constexpr ElementType BASE = 100000000ull;
    static constexpr ElementType BB = 10ull;
    static constexpr size_t LOG_BASE = 8;
    bool sign;
    size_t log_len, len;
    BlockHandle data;
    BasicIntegerType(size_t log_len);
    virtual ~BasicIntegerType();
    ElementType* get_pointer() const noexcept;
};

struct BasicComputeUnitType {
    using FuncList = std::vector<std::function<void()>>;
    FuncList forward_calls;
    BasicComputeUnitType();
    virtual ~BasicComputeUnitType();
    BasicComputeUnitType(const BasicComputeUnitType&) = default;
    BasicComputeUnitType& operator = (const BasicComputeUnitType&) = default;
    BasicComputeUnitType(BasicComputeUnitType&&) = default;
    BasicComputeUnitType& operator = (BasicComputeUnitType&&) = default;
    virtual void dependency_notice();
    virtual void forward();
    virtual void add_dependency(BasicComputeUnitType& predecessor);
};

template<typename DependencySynchronizerType>
concept ValidDependencySynchronizer = std::default_initializable<DependencySynchronizerType> &&
requires(DependencySynchronizerType dependency_synchronizer) {
    { dependency_synchronizer.initialize_as_zero() };
    { dependency_synchronizer.increment() };
    { dependency_synchronizer.ready() } -> std::convertible_to<bool>;
};

template<typename DependencySynchronizerType>
requires ValidDependencySynchronizer<DependencySynchronizerType>
struct ParallelizableUnit: public BasicComputeUnitType {
    using TaskList = putils::ThreadPool::TaskList;
    TaskList task_list;
    std::atomic<size_t> forward_synchronizer;
    DependencySynchronizerType dependency_synchronizer;
    ParallelizableUnit(): task_list(), forward_synchronizer(0), dependency_synchronizer() {
        dependency_synchronizer.initialize_as_zero();
    };
    ~ParallelizableUnit() override = default;
    void dependency_notice() override {
        try {
            if (dependency_synchronizer.ready()) {
                putils::ThreadPool::get_global_threadpool().submit(task_list);
            }
        } PUTILS_CATCH_THROW_GENERAL
    }
    void forward() override {
        if (forward_synchronizer.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            for (auto& forward_call: forward_calls) {
                try {
                    forward_call();
                } PUTILS_CATCH_THROW_GENERAL
            }
        }
    }
    void add_dependency(BasicComputeUnitType& predecessor) override {
        try {
            predecessor.forward_calls.emplace_back([this] { try { dependency_notice(); } PUTILS_CATCH_THROW_GENERAL });
            dependency_synchronizer.increment();
        } PUTILS_CATCH_THROW_GENERAL
    }
    template<typename Callable>
    void add_task_from_outer(Callable&& callable) noexcept {
        task_list.emplace_back(putils::wrap_task([callable, this] { callable(); forward(); }));
        forward_synchronizer.fetch_add(1, std::memory_order_acq_rel);
    }
};

template<typename DependencySynchronizerType>
requires ValidDependencySynchronizer<DependencySynchronizerType>
inline std::unique_ptr<ParallelizableUnit<DependencySynchronizerType>> make_parallelizable() {
    return std::make_unique<ParallelizableUnit<DependencySynchronizerType>>();
}

template<typename DependencySynchronizerType> 
requires ValidDependencySynchronizer<DependencySynchronizerType>
struct MonoUnit: public BasicComputeUnitType {
    using TaskPtr = putils::ThreadPool::TaskPtr;
    TaskPtr task;
    DependencySynchronizerType dependency_synchronizer;
    MonoUnit(): task(nullptr), dependency_synchronizer() {
        dependency_synchronizer.initialize_as_zero();
    };
    ~MonoUnit() override = default;
    void dependency_notice() override {
        try {
            if (dependency_synchronizer.ready()) {
                task->run();
            }
        } PUTILS_CATCH_THROW_GENERAL
    }
    void forward() override {
        for (auto& forward_call: forward_calls) {
            try {
                forward_call();
            } PUTILS_CATCH_THROW_GENERAL
        }
    }
    void add_dependency(BasicComputeUnitType& predecessor) override {
        try {
            predecessor.forward_calls.emplace_back([this] { try { dependency_notice(); } PUTILS_CATCH_THROW_GENERAL });
            dependency_synchronizer.increment();
        } PUTILS_CATCH_THROW_GENERAL
    }
    template<typename Callable>
    void add_task_from_outer(Callable&& callable) {
        if (task == nullptr) {
            task = putils::wrap_task([callable, this] { callable(); forward(); });
        } else {
            throw PUTILS_GENERAL_EXCEPTION("Single task unit has duplicate task initialization!", "compute unit error");
        }
    }
};

template<typename DependencySynchronizerType>
requires ValidDependencySynchronizer<DependencySynchronizerType>
inline std::unique_ptr<MonoUnit<DependencySynchronizerType>> make_mono() {
    return std::make_unique<MonoUnit<DependencySynchronizerType>>();
}

struct MultiTaskSynchronizer {
    std::atomic<size_t> synchronizer;
    void initialize_as_zero() noexcept {
        synchronizer.store(0, std::memory_order_release);
    }
    bool ready() noexcept {
        return synchronizer.fetch_sub(1, std::memory_order_acq_rel) == 1;
    }
    void increment() noexcept {
        synchronizer.fetch_add(1, std::memory_order_acq_rel);
    }
};

struct MonoSynchronizer {
    bool flag;
    void initialize_as_zero() noexcept {
        flag = false;
    }
    constexpr bool ready() const noexcept {
        return true;
    }
    void increment() {
        if (flag) {
            throw PUTILS_GENERAL_EXCEPTION("Single dependency unit has duplicate dependency initialization!", "compute unit error");
        }
    }
};

struct LatchSynchronizer {
    std::latch synchronizer;
    void add_dependency(BasicComputeUnitType& predecessor) {
        try {
            predecessor.forward_calls.emplace_back([this] { synchronizer.count_down(); });
        } PUTILS_CATCH_THROW_GENERAL
    }
};

struct BasicNodeType {
    using DataPtr = std::shared_ptr<BasicIntegerType>;
    using NodePtr = BasicNodeType*;
    using Procedure = std::list<std::unique_ptr<BasicComputeUnitType>>;
    DataPtr data;
    NodePtr next;
    Procedure procedure;
    BasicNodeType();
    virtual ~BasicNodeType();
    BasicNodeType(const BasicNodeType&) = default;
    BasicNodeType& operator = (const BasicNodeType&) = default;
    BasicNodeType(BasicNodeType&&) = default;
    BasicNodeType& operator = (BasicNodeType&&) = default;
};

struct BasicTransformation: public BasicNodeType {
    NodePtr operand;
    BasicTransformation();
    ~BasicTransformation() override;
};

struct BasicBinaryOperation: public BasicNodeType {
    NodePtr operand_A, operand_B;
    BasicBinaryOperation();
    ~BasicBinaryOperation() override;
};

struct ConstantNode: public BasicNodeType {
    bool referenced;
    ConstantNode(bool referenced = false);
    ConstantNode(const BasicNodeType& node, bool referenced = false);
    ~ConstantNode() override;
};

void parse_string_to_integer(std::string_view integer_view, BasicIntegerType& data);
std::string parse_integer_to_string(const BasicIntegerType& data) noexcept;

}
