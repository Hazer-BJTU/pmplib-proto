#pragma once

#include <bit>
#include <limits>
#include <atomic>
#include <cstdlib>
#include <shared_mutex>

#include "RuntimeLog.h"

namespace putils {

class HeadBlock;
class MemoryPool;

class MemBlock {
public:
    using uPtr = uint8_t*;
    using BlockHandle = MemBlock*;
    static constexpr size_t DEFAULT_ALIGNMENT = 64;
    static constexpr size_t DEFAULT_MIN_BLOCK_SIZE = 4096;
private:
    bool header;
    std::atomic<bool> free;
    size_t len_bytes;
    HeadBlock& head_block;
    uPtr starting;
    BlockHandle pre_block, nex_block;
    MemBlock(bool header, size_t log_len, HeadBlock& head_block);
    ~MemBlock();
    friend HeadBlock;
    friend MemoryPool;
    friend void release(MemBlock::BlockHandle&) noexcept;
public:
    MemBlock(const MemBlock&) = delete;
    MemBlock& operator = (const MemBlock&) = delete;
    MemBlock(MemBlock&&) = delete;
    MemBlock& operator = (MemBlock&&) = delete;
    template<typename Type>
    Type* get() const noexcept {
        return reinterpret_cast<Type*>(starting);
    }
    template<typename Type = uint8_t>
    size_t length() const noexcept {
        return len_bytes / sizeof(Type);
    }
};

void release(MemBlock::BlockHandle& handle) noexcept;
std::string human(size_t bytes) noexcept;

class HeadBlock {
public:
    using uPtr = uint8_t*;
    using BlockHandle = MemBlock*;
    enum class Method { FIRST_FIT, BEST_FIT };
private:
    size_t num_blocks;
    size_t total_len_bytes;
    BlockHandle first, last;
    std::mutex list_lock;
    BlockHandle internal_assign(size_t target, Method method = Method::FIRST_FIT);
    void internal_extend(size_t at_least);
    void internal_compact() noexcept;
    friend MemoryPool;
public:
    HeadBlock(size_t initial);
    ~HeadBlock();
    HeadBlock(const HeadBlock&) = delete;
    HeadBlock& operator = (const HeadBlock&) = delete;
    HeadBlock(HeadBlock&&) = delete;
    HeadBlock& operator = (HeadBlock&&) = delete;
    BlockHandle try_assign(size_t target, Method method = Method::FIRST_FIT);
    BlockHandle extend_and_assign(size_t target, Method method = Method::FIRST_FIT);
    BlockHandle compact_and_assign(size_t target, Method method = Method::FIRST_FIT);
    BlockHandle assign_and_compact(size_t target, Method method = Method::FIRST_FIT);
};

class MemoryPool {
public:
    using BlockHandle = MemBlock*;
    using HeadList = std::vector<std::unique_ptr<HeadBlock>>;
    static constexpr HeadBlock::Method FIRST_FIT = HeadBlock::Method::FIRST_FIT;
    static constexpr HeadBlock::Method BEST_FIT = HeadBlock::Method::BEST_FIT;
    struct MemView {
        size_t bytes_total;
        size_t num_blocks;
        size_t avg_block_size;
        size_t min_block_size;
        size_t max_block_size;
        size_t bytes_in_use;
        float usage_ratio;
    };
private:
    static std::random_device seed_generator;
    static size_t num_lists_reservation;
    static size_t initialization_block_size;
    static std::atomic<bool> initialized;
    static std::mutex setting_lock;
    HeadList list;
    MemoryPool();
    ~MemoryPool();
public:
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator = (const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator = (MemoryPool&&) = delete;
    static bool set_global_memorypool(
        size_t num_lists_reservsion = MemoryPool::num_lists_reservation,
        size_t initialization_block_size = MemoryPool::initialization_block_size
    ) noexcept;
    static MemoryPool& get_global_memorypool() noexcept;
    BlockHandle allocate(size_t target, HeadBlock::Method method = HeadBlock::Method::FIRST_FIT);
    MemView report() noexcept;
};

}
