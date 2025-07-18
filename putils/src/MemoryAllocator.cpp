#include "MemoryAllocator.h"

namespace putils {

MemBlock::MemBlock(bool header, size_t log_len, HeadBlock& head_block):
header(header), free(true), len_bytes(0), head_block(head_block),
starting(nullptr), pre_block(nullptr), nex_block(nullptr) {
    if (header) {
        len_bytes = 1ull << log_len;
        try {
            starting = reinterpret_cast<uPtr>(
                std::aligned_alloc(
                    DEFAULT_ALIGNMENT, 
                    std::max<size_t>(DEFAULT_MIN_BLOCK_SIZE, len_bytes)
                )
            );
            if (!starting) {
                throw std::bad_alloc();
            }
        } PUTILS_CATCH_THROW_GENERAL
    }
}

MemBlock::~MemBlock() {
    if (starting && !free.load(std::memory_order_acquire)) {
        auto& logger = RuntimeLog::get_global_log();
        std::stringstream ss;
        ss << "Block " << this << " with starting address [" 
           << static_cast<void*>(starting) << "] is never released.";
        logger.add(ss.str(), RuntimeLog::Level::WARN);
    }
    if (header) {
        if (starting) {
            std::free(starting);
            starting = nullptr;
        }
    }
}

void release(MemBlock::BlockHandle& handle) noexcept {
    handle->free.store(true, std::memory_order_release);
    handle = nullptr;
    return;
}

std::string human(size_t bytes) noexcept {
    std::stringstream ss;
    float kilo_bytes = bytes / 1024.0f;
    float mega_bytes = kilo_bytes / 1024.0f;
    float giga_bytes = mega_bytes / 1024.0f;
    if (giga_bytes >= 1.0) {
        ss << std::fixed << std::setprecision(2) << giga_bytes << "GB";
    } else if (mega_bytes >= 1.0) {
        ss << std::fixed << std::setprecision(2) << mega_bytes << "MB";
    } else if (kilo_bytes >= 1.0) {
        ss << std::fixed << std::setprecision(2) << kilo_bytes << "KB";
    } else {
        ss << bytes << "B";
    }
    return ss.str();
}

HeadBlock::HeadBlock(size_t initial): list_lock() {
    num_blocks = 1;
    first = new MemBlock(true, std::countr_zero(std::bit_ceil(initial)), *this);
    total_len_bytes = first->len_bytes;
    last = first;
}

HeadBlock::~HeadBlock() {
    for (
        BlockHandle p = first, np = p ? p->nex_block : nullptr; p;
        p = np /* P = NP :-) */, np = p ? p->nex_block : nullptr
    ) {
        delete p;
        p = nullptr;
    }
}

HeadBlock::BlockHandle HeadBlock::internal_assign(size_t target, Method method) {
    BlockHandle chosen = nullptr;
    size_t safe_target = (target + MemBlock::DEFAULT_ALIGNMENT - 1) / MemBlock::DEFAULT_ALIGNMENT * MemBlock::DEFAULT_ALIGNMENT;
    if (method == Method::FIRST_FIT) {
        for (BlockHandle p = first; p; p = p->nex_block) {
            if (p->free.load(std::memory_order_acquire) && p->len_bytes >= safe_target) {
                chosen = p;
                break;
            }
        }
    } else if (method == Method::BEST_FIT) {
        size_t min_gap = std::numeric_limits<size_t>::max();
        for (BlockHandle p = first; p; p = p->nex_block) {
            if (p->free.load(std::memory_order_acquire) && p->len_bytes >= safe_target) {
                if (p->len_bytes - safe_target < min_gap) {
                    min_gap = p->len_bytes - safe_target;
                    chosen = p;
                }
            }
        }
    }
    if (!chosen) {
        return nullptr;
    }
    if (chosen->len_bytes == safe_target) {
        chosen->free.store(false, std::memory_order_release);
        return chosen;
    }
    try {
        num_blocks++;
        BlockHandle new_block = new MemBlock(false, 0, *this);
        new_block->len_bytes = chosen->len_bytes - safe_target;
        new_block->starting = chosen->starting + safe_target;
        new_block->pre_block = chosen;
        new_block->nex_block = chosen->nex_block;
        if (chosen->nex_block) {
            chosen->nex_block->pre_block = new_block;
        }
        chosen->nex_block = new_block;
        chosen->len_bytes = safe_target;
        chosen->free.store(false, std::memory_order_release);
        if (chosen == last) {
            last = new_block;
        }
        return chosen;
    } PUTILS_CATCH_THROW_GENERAL
}

void HeadBlock::internal_extend(size_t at_least) {
    size_t new_bytes = std::bit_ceil(std::max<size_t>(at_least, total_len_bytes));
    try {
        num_blocks++;
        BlockHandle new_block = new MemBlock(true, std::countr_zero(new_bytes), *this);
        total_len_bytes += new_block->len_bytes;
        new_block->pre_block = last;
        last->nex_block = new_block;
        last = new_block;
    } PUTILS_CATCH_THROW_GENERAL
    return;
}

void HeadBlock::internal_compact() noexcept {
    BlockHandle curr = first;
    while(curr && curr->nex_block) {
        BlockHandle neighbor = curr->nex_block;
        if (
            curr->free.load(std::memory_order_acquire) &&
            neighbor->free.load(std::memory_order_acquire) && 
            !neighbor->header
        ) {
            curr->len_bytes += neighbor->len_bytes;
            curr->nex_block = neighbor->nex_block;
            if (neighbor->nex_block) {
                neighbor->nex_block->pre_block = curr;
            }
            num_blocks--;
            if (neighbor == last) {
                last = curr;
            }
            delete neighbor;
            neighbor = nullptr;
        } else {
            curr = neighbor;
        }
    }
    return;
}

HeadBlock::BlockHandle HeadBlock::try_assign(size_t target, Method method) {
    std::lock_guard<std::mutex> lock(list_lock);
    try {
        return internal_assign(target, method);
    } PUTILS_CATCH_THROW_GENERAL
}

HeadBlock::BlockHandle HeadBlock::extend_and_assign(size_t target, Method method) {
    std::lock_guard<std::mutex> lock(list_lock);
    try {
        internal_extend(target);
        return internal_assign(target, method);
    } PUTILS_CATCH_THROW_GENERAL
}

HeadBlock::BlockHandle HeadBlock::compact_and_assign(size_t target, Method method) {
    std::lock_guard<std::mutex> lock(list_lock);
    try {
        internal_compact();
        return internal_assign(target, method);
    } PUTILS_CATCH_THROW_GENERAL
}

HeadBlock::BlockHandle HeadBlock::assign_and_compact(size_t target, Method method) {
    std::lock_guard<std::mutex> lock(list_lock);
    try {
        BlockHandle handle = internal_assign(target, method);
        if (handle) {
            return handle;
        } else {
            internal_compact();
            return nullptr;
        }
    } PUTILS_CATCH_THROW_GENERAL
}

std::random_device MemoryPool::seed_generator;
size_t MemoryPool::num_lists_reservation = 16;
size_t MemoryPool::initialization_block_size = 4194304;
std::atomic<bool> MemoryPool::initialized = false;
std::mutex MemoryPool::setting_lock;

MemoryPool::MemoryPool() {
    std::lock_guard<std::mutex> lock(setting_lock);
    MemoryPool::initialized.store(true, std::memory_order_release);
    try {
        for (size_t i = 0; i < MemoryPool::num_lists_reservation; i++) {
            list.push_back(std::make_unique<HeadBlock>(MemoryPool::initialization_block_size));
        }
    } PUTILS_CATCH_THROW_GENERAL
}

MemoryPool::~MemoryPool() {}

bool MemoryPool::set_global_memorypool(size_t num_lists_reservation, size_t initialization_block_size) noexcept {
    auto& logger = RuntimeLog::get_global_log();
    std::lock_guard<std::mutex> lock(MemoryPool::setting_lock);
    if (MemoryPool::initialized.load(std::memory_order_acquire)) {
        logger.add("(MemoryPool): Settings cannot be modified after the instance has been initialized.");
        return false;
    }
    if (num_lists_reservation == 0 || initialization_block_size == 0) {
        logger.add("(MemoryPool): All numeric arguments must be positive integers.");
        return false;
    }
    MemoryPool::num_lists_reservation = num_lists_reservation;
    MemoryPool::initialization_block_size = initialization_block_size;
    size_t total = MemoryPool::num_lists_reservation * std::max<size_t>(MemoryPool::initialization_block_size, MemBlock::DEFAULT_MIN_BLOCK_SIZE);
    std::stringstream ss;
    ss << "(MemoryPool): initialization storage size: " << human(total);
    logger.add(ss.str());
    return true;
}

MemoryPool& MemoryPool::get_global_memorypool() noexcept {
    static MemoryPool memorypool_instance;
    return memorypool_instance;
}

MemoryPool::BlockHandle MemoryPool::allocate(size_t target, HeadBlock::Method method) {
    thread_local std::mt19937 gen(MemoryPool::seed_generator());
    thread_local std::uniform_int_distribution<size_t> udist(0, MemoryPool::num_lists_reservation - 1);
    size_t starting_idx = udist(gen);
    try {
        for (
            size_t idx = starting_idx, nex_idx = idx + 1 == MemoryPool::num_lists_reservation ? 0 : idx + 1;
            nex_idx != starting_idx;
            idx = nex_idx, nex_idx = idx + 1 == MemoryPool::num_lists_reservation ? 0 : idx + 1
        ) {
            BlockHandle handle = list[idx]->assign_and_compact(target, method);
            if (handle) {
                return handle;
            }
        }
        return list[starting_idx]->extend_and_assign(target, method);
    } PUTILS_CATCH_THROW_GENERAL
}

MemoryPool::MemView MemoryPool::report() noexcept {
    MemView view = {0, 0, 0, std::numeric_limits<size_t>::max(), 0, 0};
    for (auto& head: list) {
        std::lock_guard<std::mutex> lock(head->list_lock);
        for (BlockHandle p = head->first; p; p = p->nex_block) {
            view.bytes_total += p->len_bytes;
            view.num_blocks++;
            view.max_block_size = std::max<size_t>(view.max_block_size, p->len_bytes);
            view.min_block_size = std::min<size_t>(view.min_block_size, p->len_bytes);
            if (p->free.load(std::memory_order_acquire)) {
                view.bytes_in_use += p->len_bytes;
            }
        }
    }
    view.avg_block_size = view.bytes_total / std::max<size_t>(view.num_blocks, 1);
    view.usage_ratio = view.bytes_in_use * 1.0f / std::max<size_t>(view.bytes_total, 1);
    return view;
}

}
