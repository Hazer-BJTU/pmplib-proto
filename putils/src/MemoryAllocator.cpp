#include "MemoryAllocator.h"

namespace putils {

MemBlock::BlockHandle MemBlock::split(BlockHandle handle, size_t target) {
    if (!handle || !handle->free.load(std::memory_order_acquire)) {
        return nullptr;
    }
    /* size_t safe_target = (target + DEFAULT_ALIGNMENT - 1) / DEFAULT_ALIGNMENT * DEFAULT_ALIGNMENT; */
    if (handle->len_bytes < target) {
        return nullptr;
    } else if (handle->len_bytes == target) {
        handle->free.store(false, std::memory_order_release);
        return handle;
    } else if (handle->len_bytes > target) {
        try {
            BlockHandle new_handle = std::make_shared<MemBlock>(false);
            new_handle->len_bytes = handle->len_bytes - target;
            new_handle->starting = handle->starting + target;
            new_handle->next_block = handle->next_block;
            new_handle->pre_block = handle.get();
            new_handle->list_lock = handle->list_lock;
            if (handle->next_block) {
                handle->next_block->pre_block = new_handle.get();
            }
            handle->next_block = new_handle;
            handle->len_bytes = target;
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
        if (p != handle) {
            p->valid.store(false, std::memory_order_release);
        }
    }
    BlockHandle delete_ptr = handle->next_block;
    while(delete_ptr != p) {
        auto tmp = delete_ptr->next_block;
        delete_ptr->next_block.reset();
        delete_ptr = tmp;
    }
    handle->next_block = p;
    if (p) {
        p->pre_block = handle.get();
    }
    handle->len_bytes = accum;
    return p;
}

MemBlock::MemBlock(bool header, size_t log_size):
header(header), free(true), valid(true), len_bytes(1u << log_size), starting(nullptr),
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
                ss << "Block " << p << " with starting address [" 
                   << static_cast<void*>(p->starting) << "] is never released.";
                logger.add(ss.str(), RuntimeLog::Level::WARN);
            }
        }
        if (starting) {
            std::free(starting);
            starting = nullptr;
        }
        BlockHandle delete_ptr = next_block;
        while(delete_ptr) {
            auto tmp = delete_ptr->next_block;
            delete_ptr->next_block.reset();
            delete_ptr = tmp;
        }
        next_block.reset();
    }
}

MemBlock::BlockHandle MemBlock::make_head(size_t at_least) {
    size_t min_power = 0;
    while((1u << min_power) < std::max<size_t>(at_least, DEFAULT_MIN_BLOCK_SIZE)) {
        min_power++;
    }
    return std::make_shared<MemBlock>(true, min_power);
}

MemBlock::BlockHandle MemBlock::assign(BlockHandle head, size_t target, Method method) {
    if (!head || !head->header) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(*head->list_lock);
    size_t safe_target = (target + DEFAULT_ALIGNMENT - 1) / DEFAULT_ALIGNMENT * DEFAULT_ALIGNMENT;
    if (method == Method::FIRST_FIT) {
        for (BlockHandle p = head; p; p = p->next_block) {
            try {
                BlockHandle assigned = split(p, safe_target);
                if (assigned) {
                    return assigned;
                }
            } PUTILS_CATCH_THROW_GENERAL
        }
    } else if (method == Method::BEST_FIT) {
        std::vector<std::pair<BlockHandle, size_t>> chosen_blocks;
        for (BlockHandle p = head; p; p = p->next_block) {
            if (p->len_bytes >= safe_target) {
                chosen_blocks.push_back(std::make_pair(p, p->len_bytes - safe_target));
            }
        }
        std::sort(
            chosen_blocks.begin(), 
            chosen_blocks.end(), 
            [](const auto& x, const auto& y) {
                return x.second < y.second; 
            }
        );
        for (auto& [block, gap]: chosen_blocks) {
            try {
                BlockHandle assigned = split(block, safe_target);
                if (assigned) {
                    return assigned;
                }
            } PUTILS_CATCH_THROW_GENERAL
        }
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
    return valid.load(std::memory_order_acquire) ? len_bytes : 0;
}

void MemBlock::release() noexcept {
    free.store(true, std::memory_order_release);
}

std::random_device MemoryPool::seed_generator;
size_t MemoryPool::num_blocks_reservation = 8;
std::atomic<bool> MemoryPool::initialized = false;
std::mutex MemoryPool::setting_lock;

MemoryPool::MemoryPool(): list_lock(), block_list() {
    std::lock_guard<std::mutex> lock(MemoryPool::setting_lock);
    MemoryPool::initialized.store(true, std::memory_order_release);
    for (size_t i = 0; i < MemoryPool::num_blocks_reservation; i++) {
        block_list.push_back(MemBlock::make_head(MemBlock::DEFAULT_BLOCK_SIZE));
    }
}

MemoryPool::~MemoryPool() {}

bool MemoryPool::set_global_memorypool(size_t num_blocks_reservation) noexcept {
    auto& logger = RuntimeLog::get_global_log();
    std::lock_guard<std::mutex> lock(MemoryPool::setting_lock);
    if (MemoryPool::initialized.load(std::memory_order_acquire)) {
        logger.add("(MemoryPool): Settings cannot be modified after the instance has been initialized.");
        return false;
    }
    if (num_blocks_reservation == 0) {
        logger.add("(MemoryPool): All numeric arguments must be positive integers.");
        return false;
    }
    MemoryPool::num_blocks_reservation = num_blocks_reservation;

    std::stringstream ss;
    size_t bytes = MemoryPool::num_blocks_reservation * MemBlock::DEFAULT_BLOCK_SIZE;
    size_t kilobytes = bytes / 1024;
    size_t megabytes = kilobytes / 1024;
    size_t gigabytes = megabytes / 1024;
    ss << "(MemoryPool): potential memory usage ";
    if (gigabytes > 0) {
        ss << gigabytes << "GB";
    } else if (megabytes > 0) {
        ss << megabytes << "MB";
    } else if (kilobytes > 0) {
        ss << kilobytes << "KB";
    } else {
        ss << bytes << "B";
    }
    logger.add(ss.str());
    return true;
}

MemoryPool& MemoryPool::get_global_memorypool() noexcept {
    static MemoryPool memorypool_instance;
    return memorypool_instance;
}

}
