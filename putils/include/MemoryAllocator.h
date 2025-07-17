#pragma once

#include <bit>
#include <limits>
#include <atomic>
#include <cstdlib>
#include <shared_mutex>

#include "RuntimeLog.h"

namespace putils {

class MemoryPool;

class MemBlock {
public:
    using uPtr = uint8_t*;
    using BlockHandle = std::shared_ptr<MemBlock>;
    using BlockPtr = MemBlock*;
    enum class Method {
        FIRST_FIT = 0,
        BEST_FIT = 1
    };
private:
    static constexpr size_t DEFAULT_ALIGNMENT = 64;
    static constexpr size_t DEFAULT_BLOCK_SIZE = 4194304;
    static constexpr size_t DEFAULT_MIN_BLOCK_SIZE = 4096;
    bool header;
    std::atomic<bool> free, valid;
    size_t len_bytes;
    uPtr starting;
    BlockHandle next_block;
    BlockPtr pre_block;
    std::shared_ptr<std::mutex> list_lock;
    static BlockHandle split(BlockHandle handle, size_t target);
    static BlockHandle merge(BlockHandle handle) noexcept;
    static BlockHandle internal_assign(
        BlockHandle head,
        size_t target,
        Method method = Method::FIRST_FIT
    );
    friend MemoryPool;
public:
    MemBlock(
        bool header = false,
        size_t log_size = std::countr_zero(DEFAULT_BLOCK_SIZE)
    );
    ~MemBlock();
    MemBlock(const MemBlock&) = delete;
    MemBlock& operator = (const MemBlock&) = delete;
    MemBlock(MemBlock&&) = delete;
    MemBlock& operator = (MemBlock&&) = delete;
    static BlockHandle make_head(size_t at_least);
    static BlockHandle assign(
        BlockHandle head, 
        size_t target, 
        Method method = Method::FIRST_FIT
    );
    static void compact(BlockHandle head) noexcept;
    static BlockHandle compact_and_assign(
        BlockHandle head,
        size_t target,
        Method method = Method::FIRST_FIT
    );
    template<typename Type>
    Type* get() const noexcept {
        return valid.load(std::memory_order_acquire) ? 
        reinterpret_cast<Type*>(starting) : nullptr;
    }
    size_t length() const noexcept;
    void release() noexcept;
};

class MemoryPool {
public:
    struct MemView {
        size_t bytes_in_use;
        size_t bytes_total;
        size_t max_block_size;
        size_t min_block_size;
        size_t avg_block_size;
        size_t num_blocks;
        float use_ratio;
    };
private:
    using BlockHandle = std::shared_ptr<MemBlock>;
    using BlockList = std::vector<BlockHandle>;
    static std::random_device seed_generator;
    static size_t num_blocks_reservation;
    static size_t new_blocks_size;
    static std::atomic<bool> initialized;
    static std::mutex setting_lock;
    std::shared_mutex list_lock;
    BlockList block_list;
    MemoryPool();
    ~MemoryPool();
    MemoryPool::BlockHandle extend_and_allocate(size_t target);
    BlockHandle try_allocate(
        size_t block_idx,
        size_t target,
        MemBlock::Method method = MemBlock::Method::FIRST_FIT
    );
public:
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator = (const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator = (MemoryPool&&) = delete;
    static bool set_global_memorypool(
        size_t num_blocks_reservation = MemoryPool::num_blocks_reservation,
        size_t new_blocks_size = MemoryPool::new_blocks_size
    ) noexcept;
    static MemoryPool& get_global_memorypool() noexcept;
    BlockHandle allocate(
        size_t target,
        MemBlock::Method method = MemBlock::Method::FIRST_FIT
    );
    MemView report() noexcept;
};

}
