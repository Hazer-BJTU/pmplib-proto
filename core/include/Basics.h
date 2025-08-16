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

/**
 * @class BasicIntegerType
 * @brief A high-precision integer type that stores data in large base for efficiency.
 *
 * This class implements a variable-length integer type that:
 * - Uses a large base (100000000) for efficient storage
 * - Dynamically allocates memory from a global memory pool
 * - Automatically manages memory lifecycle
 * - Provides direct pointer access for high performance
 * - Enforces configurable length boundaries
 *
 * @tparam ElementType The underlying storage type (uint64_t)
 * @tparam BlockHandle Memory pool block handle type
 *
 * @note Memory Management:
 * - Uses putils::MemoryPool for efficient allocation
 * - Memory pool uses chunked design with length-based indexing
 * - Automatically releases memory in destructor
 * - Provides direct pointer access via get_pointer()
 *
 * @throw PUTILS_GENERAL_EXCEPTION
 *       Throws if configuration values are invalid
 * @throw Various memory allocation exceptions
 *
 * @warning Direct pointer access requires careful usage to avoid memory issues
 * 
 * @author Hazer
 * @date 2025/8/15
 */

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

/**
 * @class BasicComputeUnitType
 * @brief Base class for DAG-based multi-threaded task scheduling units.
 *
 * This class provides the fundamental infrastructure for creating directed acyclic graphs (DAGs)
 * of computational tasks with dependency tracking. The scheduling flow follows:
 * 1. dependency_notice() is called when predecessors complete
 * 2. Checks if all dependencies are satisfied
 * 3. If ready, executes the unit's tasks
 * 4. Calls forward() to propagate completion to successor nodes
 *
 * @note The class is designed for inheritance with two concrete implementations provided:
 *       - ParallelizableUnit (for parallel task groups)
 *       - MonoUnit (for single tasks)
 *
 * @var forward_calls
 *      Collection of functions to notify dependent units
 *
 * @warning Not thread-safe for concurrent modification. Dependencies should be established
 *          during graph construction phase before execution begins.
 */

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

/**
 * @class ParallelizableUnit
 * @brief Compute unit that manages a group of parallelizable tasks.
 *
 * This implementation:
 * - Manages multiple tasks through a ThreadPool::TaskList
 * - Uses atomic counters for synchronization
 * - Provides non-blocking execution
 * - Supports arbitrary dependency counts
 *
 * @tparam DependencySynchronizerType Synchronization policy (must satisfy ValidDependencySynchronizer)
 *
 * @var task_list
 *      Collection of tasks to execute when dependencies are satisfied
 * @var forward_synchronizer
 *      Atomic counter for coordinating task completion
 * @var dependency_synchronizer
 *      Policy-based dependency tracking component
 *
 * @note The forward() implementation uses atomic operations to ensure proper
 *       sequencing of successor notifications after all tasks complete.
 */

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
        return;
    }
    void forward() override {
        if (forward_synchronizer.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            for (auto& forward_call: forward_calls) {
                try {
                    forward_call();
                } PUTILS_CATCH_THROW_GENERAL
            }
        }
        return;
    }
    void add_dependency(BasicComputeUnitType& predecessor) override {
        try {
            predecessor.forward_calls.emplace_back([this] { try { dependency_notice(); } PUTILS_CATCH_THROW_GENERAL });
            dependency_synchronizer.increment();
        } PUTILS_CATCH_THROW_GENERAL
        return;
    }
    template<typename Callable>
    void add_task_from_outer(Callable&& callable) noexcept {
        task_list.emplace_back(putils::wrap_task([callable, this] { callable(); forward(); }));
        forward_synchronizer.fetch_add(1, std::memory_order_acq_rel);
        return;
    }
};

/**
 * @class MonoUnit
 * @brief Compute unit optimized for single-task execution.
 *
 * This implementation:
 * - Manages exactly one task
 * - Supports thread-binding optimization (MPENGINE_THREAD_BINDING_OPTIMIZATION)
 * - Reduces synchronization overhead
 *
 * @tparam DependencySynchronizerType Synchronization policy (must satisfy ValidDependencySynchronizer)
 *
 * @var task
 *      The single task to execute
 * @var dependency_synchronizer
 *      Policy-based dependency tracking component
 *
 * @warning When MPENGINE_THREAD_BINDING_OPTIMIZATION is enabled, tasks execute
 *          directly on the calling thread which may increase call stack pressure.
 */

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
                #ifdef MPENGINE_THREAD_BINDING_OPTIMIZATION
                    task->run();
                #else
                    putils::ThreadPool::get_global_threadpool().submit(task);
                #endif
            }
        } PUTILS_CATCH_THROW_GENERAL
        return;
    }
    void forward() override {
        for (auto& forward_call: forward_calls) {
            try {
                forward_call();
            } PUTILS_CATCH_THROW_GENERAL
        }
        return;
    }
    void add_dependency(BasicComputeUnitType& predecessor) override {
        try {
            predecessor.forward_calls.emplace_back([this] { try { dependency_notice(); } PUTILS_CATCH_THROW_GENERAL });
            dependency_synchronizer.increment();
        } PUTILS_CATCH_THROW_GENERAL
        return;
    }
    template<typename Callable>
    void add_task_from_outer(Callable&& callable) {
        if (task == nullptr) {
            task = putils::wrap_task([callable, this] { callable(); forward(); });
        } else {
            throw PUTILS_GENERAL_EXCEPTION("Single task unit has duplicate task initialization!", "compute unit error");
        }
        return;
    }
};

struct MultiTaskSynchronizer {
    std::atomic<size_t> synchronizer;
    void initialize_as_zero() noexcept {
        synchronizer.store(0, std::memory_order_release);
        return;
    }
    bool ready() noexcept {
        return synchronizer.fetch_sub(1, std::memory_order_acq_rel) == 1;
    }
    void increment() noexcept {
        synchronizer.fetch_add(1, std::memory_order_acq_rel);
        return;
    }
};

struct MonoSynchronizer {
    bool flag;
    void initialize_as_zero() noexcept {
        flag = false;
        return;
    }
    constexpr bool ready() const noexcept {
        return true;
    }
    void increment() {
        if (flag) {
            throw PUTILS_GENERAL_EXCEPTION("Single dependency unit has duplicate dependency initialization!", "compute unit error");
        }
        flag = true;
        return;
    }
};

template<typename FinalSynchronizer = std::latch>
void add_dependency(FinalSynchronizer& synchronizer, BasicComputeUnitType& predecessor) noexcept {
    predecessor.forward_calls.emplace_back([&synchronizer] { synchronizer.count_down(); });
    return;
}

/**
 * @class BasicNodeType
 * @brief Fundamental computational node type managing data, dependencies, and execution flow.
 *
 * This class serves as a building block for constructing computational graphs with:
 * - Managed data storage (BasicIntegerType)
 * - Explicit dependency chaining (next pointer)
 * - Ordered sequence of computational units (Procedure)
 *
 * @note Key Features:
 * - Supports dynamic creation of both data and computation procedures
 * - Designed for serialization/deserialization to enable:
 *   * Persistent storage
 *   * Distributed computation
 *   * Checkpointing
 * - Default copy/move operations maintain structural integrity
 *
 * @typedef DataPtr
 *        Shared pointer to managed data (BasicIntegerType)
 * @typedef NodePtr
 *        Raw pointer for lightweight dependency chaining
 * @typedef Procedure
 *        Sequence of owned computational units (BasicComputeUnitType)
 *
 * @var data
 *      Managed high-precision numerical data
 * @var next
 *      Pointer to dependent node in computational graph
 * @var procedure
 *      Ordered collection of computational units to execute
 *
 * @warning NodePtr uses raw pointers - lifetime must be managed externally
 * @warning Procedure units are executed in list order - sequence matters
 */

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
