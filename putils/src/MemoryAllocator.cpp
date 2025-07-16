#include "MemoryAllocator.h"

namespace putils {

MemBlock::BlockHandle MemBlock::split(BlockHandle handle, size_t target) {
    if (!handle || !handle->free.load(std::memory_order_acquire)) {
        return nullptr;
    }
    size_t safe_target = (target + DEFAULT_ALIGNMENT - 1) / DEFAULT_ALIGNMENT * DEFAULT_ALIGNMENT;
    if (handle->len_bytes < safe_target) {
        return nullptr;
    } else if (handle->len_bytes == safe_target) {
        handle->free.store(false, std::memory_order_release);
        return handle;
    } else if (handle->len_bytes > safe_target) {
        try {
            BlockHandle new_handle = std::make_shared<MemBlock>(false);
            new_handle->len_bytes = handle->len_bytes - safe_target;
            new_handle->starting = handle->starting + safe_target;
            new_handle->next_block = handle->next_block;
            new_handle->pre_block = handle.get();
            new_handle->list_lock = handle->list_lock;
            if (handle->next_block) {
                handle->next_block->pre_block = new_handle.get();
            }
            handle->next_block = new_handle;
            handle->len_bytes = safe_target;
            handle->free.store(false, std::memory_order_release);
            return handle;
        } PUTILS_CATCH_THROW_GENERAL
    }
    return nullptr;
}

MemBlock::BlockHandle MemBlock::merge(BlockHandle handle) noexcept {
    if (!handle) {
        return nullptr;
    }
    if (!handle->free.load(std::memory_order_acquire)) {
        return handle->next_block;
    }
    size_t accum = 0;
    BlockHandle p = handle;
    for (; p && p->free.load(std::memory_order_acquire); p = p->next_block) {
        accum += p->len_bytes;
    }
    handle->next_block = p;
    if (p) {
        p->pre_block = handle.get();
    }
    handle->len_bytes = accum;
    return p;
}

MemBlock::MemBlock(bool header, size_t log_size):
header(header), free(true), len_bytes(1u << log_size), starting(nullptr),
next_block(nullptr), pre_block(nullptr), list_lock(nullptr) {
    if (header) {
        try {
            if (len_bytes < DEFAULT_MIN_BLOCK_SIZE) {
                std::stringstream ss;
                ss << "Block size must be at least " << DEFAULT_MIN_BLOCK_SIZE << ".";
                throw PUTILS_GENERAL_EXCEPTION(ss.str(), "invalid argument");
            }
            starting = reinterpret_cast<uPtr>(std::aligned_alloc(DEFAULT_ALIGNMENT, len_bytes));
            if (!starting) {
                throw std::bad_alloc();
            }
            list_lock = std::make_shared<std::mutex>();
        } PUTILS_CATCH_THROW_GENERAL
    } else {
        len_bytes = 0;
    }
}

MemBlock::~MemBlock() {
    if (header) {
        auto& logger = RuntimeLog::get_global_log();
        for (BlockPtr p = this; p; p = p->next_block.get()) {
            if (!p->free.load(std::memory_order_acquire)) {
                std::stringstream ss;
                ss << "Block " << p << " with starting address [" << p->starting << "] is never released.";
                logger.add(ss.str(), RuntimeLog::Level::WARN);
            }
        }
        if (starting) {
            std::free(starting);
            starting = nullptr;
        }
    }
}

MemBlock::BlockHandle MemBlock::make_head(size_t at_least) {
    size_t min_power = 0;
    while((1u << min_power) < std::max<size_t>(at_least, DEFAULT_MIN_BLOCK_SIZE)) {
        min_power++;
    }
    return std::make_shared<MemBlock>(true, min_power);
}

MemBlock::BlockHandle MemBlock::assign(BlockHandle head, size_t target) {
    if (!head || !head->header) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(*head->list_lock);
    for (BlockHandle p = head; p; p = p->next_block) {
        try {
            BlockHandle assigned = split(p, target);
            if (assigned) {
                return assigned;
            }
        } PUTILS_CATCH_THROW_GENERAL
    }
    return nullptr;
}

void MemBlock::compact(BlockHandle head) noexcept {
    if (!head || !head->header) {
        return;
    }
    std::lock_guard<std::mutex> lock(*head->list_lock);
    for (BlockHandle p = head; p; p = merge(p));
    return;
}

size_t MemBlock::length() const noexcept {
    return !free.load(std::memory_order_acquire) ? len_bytes : 0;
}

void MemBlock::release() noexcept {
    free.store(true, std::memory_order_release);
}

}
