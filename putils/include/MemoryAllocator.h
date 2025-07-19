#pragma once

#include <map>
#include <bit>
#include <limits>
#include <atomic>
#include <cstdlib>

#include "RuntimeLog.h"

namespace putils {

std::string human(size_t bytes) noexcept;

class MetaBlock;
class MemoryPool;

class MemBlock {
public:
    using uPtr = uint8_t*;
    using BlockHandle = std::shared_ptr<MemBlock>;
    using WeakHandle = std::weak_ptr<MemBlock>;
private:
    static constexpr size_t DEFAULT_ALIGNMENT = 64;
    static constexpr size_t DEFAULT_LOG_LEN_LOWER_BOUND = 12;
    static constexpr size_t DEFAULT_LOG_LEN_UPPER_BOUND = 32;
    bool header, free, valid;
    size_t len_bytes;
    MetaBlock& meta_block;
    uPtr starting;
    BlockHandle nex_block;
    WeakHandle pre_block;
    friend MetaBlock;
    friend MemoryPool;
    friend void release(MemBlock::BlockHandle& handle) noexcept;
public:
    MemBlock(bool header, size_t log_len, MetaBlock& meta_block);
    ~MemBlock();
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

class MetaBlock {
public:
    using BlockHandle = std::shared_ptr<MemBlock>;
    using WeakHandle = std::weak_ptr<MemBlock>;
    using BlockLenIndex = std::multimap<size_t, BlockHandle>;
private:
    static constexpr size_t DEFAULT_INIT_BLOCK_LOG_LEN = 22;
    BlockHandle first, last;
    BlockLenIndex block_len_index;
    std::mutex list_lock;
    BlockHandle internal_assign(size_t target);
    void internal_compact(const BlockHandle& handle) noexcept;
    void internal_extend(size_t at_least);
    friend MemBlock;
    friend MemoryPool;
    friend void release(MemBlock::BlockHandle& handle) noexcept;
public:
    MetaBlock(size_t at_least = 1ull << DEFAULT_INIT_BLOCK_LOG_LEN);
    ~MetaBlock();
    MetaBlock(const MetaBlock&) = delete;
    MetaBlock& operator = (const MetaBlock&) = delete;
    MetaBlock(MetaBlock&&) = delete;
    MetaBlock& operator = (MetaBlock&&) = delete;
    BlockHandle try_assign(size_t target);
    BlockHandle extend_and_assign(size_t target);
};

void release(MemBlock::BlockHandle& handle) noexcept;

class MemoryPool {
public:
    using BlockHandle = std::shared_ptr<MemBlock>;
    using MetaList = std::vector<std::unique_ptr<MetaBlock>>;
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
    MetaList list;
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
    BlockHandle allocate(size_t target);
    MemView report() noexcept;
    std::string memory_distribution() noexcept;
};

}
