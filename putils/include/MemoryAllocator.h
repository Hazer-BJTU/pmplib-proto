#pragma once

#include <bit>
#include <atomic>
#include <cstdlib>

#include "RuntimeLog.h"

namespace putils {

class MemBlock {
public:
    using uPtr = uint8_t*;
    using BlockHandle = std::shared_ptr<MemBlock>;
    using BlockPtr = MemBlock*;
private:
    static constexpr size_t DEFAULT_ALIGNMENT = 64;
    static constexpr size_t DEFAULT_BLOCK_SIZE = 4194304;
    static constexpr size_t DEFAULT_MIN_BLOCK_SIZE = 4096;
    bool header;
    std::atomic<bool> free;
    size_t len_bytes;
    uPtr starting;
    BlockHandle next_block;
    BlockPtr pre_block;
    std::shared_ptr<std::mutex> list_lock;
    static BlockHandle split(BlockHandle handle, size_t target);
    static BlockHandle merge(BlockHandle handle) noexcept;
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
    static BlockHandle assign(BlockHandle head, size_t target);
    static void compact(BlockHandle head) noexcept;
    template<typename Type>
    Type* get() const noexcept {
        return !free.load(std::memory_order_acquire) ? 
        reinterpret_cast<Type*>(starting) : nullptr;
    }
    size_t length() const noexcept;
    void release() noexcept;
};

}
