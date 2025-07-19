#include "MemoryAllocator.h"

namespace putils {

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

MemBlock::MemBlock(bool header, size_t log_len, MetaBlock& meta_block):
header(header), free(true), valid(true), len_bytes(0), meta_block(meta_block),
starting(nullptr), nex_block(nullptr), pre_block() {
    if (header) {
        try {
            if (log_len > DEFAULT_LOG_LEN_UPPER_BOUND) {
                auto& logger = RuntimeLog::get_global_log();
                std::stringstream ss;
                ss << "Memory allocation request too large: 2^" << log_len << " bytes "
                   << "exceeds maximum allowed 2^" << DEFAULT_LOG_LEN_UPPER_BOUND << " bytes.";
                logger.add(ss.str(), RuntimeLog::Level::WARN);
            }
            log_len = std::max<size_t>(std::min<size_t>(log_len, DEFAULT_LOG_LEN_UPPER_BOUND), DEFAULT_LOG_LEN_LOWER_BOUND);
            starting = reinterpret_cast<uPtr>(std::aligned_alloc(DEFAULT_ALIGNMENT, len_bytes = 1ull << log_len));
            if (!starting) {
                throw std::bad_alloc();
            }
        } PUTILS_CATCH_THROW_GENERAL
    }
}

MemBlock::~MemBlock() {
    if (valid && !free) {
        auto& logger = RuntimeLog::get_global_log();
        std::stringstream ss;
        ss << "Block " << this << " with starting address [" << starting << "] is never released.";
        logger.add(ss.str(), RuntimeLog::Level::WARN);
    }
    if (header && starting) {
        std::free(starting);
        starting = nullptr;
    }
}

MetaBlock::MetaBlock(size_t at_least): first(nullptr), last(nullptr), block_len_index(), list_lock() {
    try {
        first = std::make_shared<MemBlock>(true, std::countr_zero(std::bit_ceil(at_least)), *this);
        last = first;
        block_len_index.insert(std::make_pair(last->len_bytes, last));
    } PUTILS_CATCH_THROW_GENERAL
}

MetaBlock::~MetaBlock() {
    std::lock_guard<std::mutex> lock(list_lock);
    BlockHandle p = first, np;
    while(p) {
        np = p->nex_block;
        p->nex_block.reset();
        p = np; /* P = NP :-) */
    }
}

MetaBlock::BlockHandle MetaBlock::internal_assign(size_t target) {
    size_t safe_target = (target + MemBlock::DEFAULT_ALIGNMENT - 1) / MemBlock::DEFAULT_ALIGNMENT * MemBlock::DEFAULT_ALIGNMENT;
    while(true) {
        auto it = block_len_index.lower_bound(safe_target);
        if (it == block_len_index.end()) {
            return nullptr;
        }
        BlockHandle handle = it->second;
        block_len_index.erase(it);
        if (!handle || !handle->free || !handle->valid) {
            continue;
        }
        if (handle->len_bytes == safe_target) {
            handle->free = false;
            return handle;
        } else if (handle->len_bytes > safe_target) {
            BlockHandle new_handle = std::make_shared<MemBlock>(false, 0, *this);
            new_handle->len_bytes = handle->len_bytes - safe_target;
            new_handle->starting = handle->starting + safe_target;
            new_handle->nex_block = handle->nex_block;
            new_handle->pre_block = handle;
            if (handle->nex_block) {
                handle->nex_block->pre_block = new_handle;
            }
            block_len_index.insert(std::make_pair(new_handle->len_bytes, new_handle));
            handle->len_bytes = safe_target;
            handle->free = false;
            handle->nex_block = new_handle;
            if (handle == last) {
                last = new_handle;
            }
            return handle;
        }
    }
}

void MetaBlock::internal_compact(const BlockHandle& handle) noexcept {
    if (!handle || !handle->free) {
        return;
    }
    BlockHandle curr = handle, pre, nex;
    while((pre = curr->pre_block.lock()) && pre->free && !curr->header) {
        curr = pre;
    }
    while(curr->nex_block && curr->nex_block->free && !curr->nex_block->header) {
        nex = curr->nex_block;
        curr->len_bytes += nex->len_bytes;
        curr->nex_block = nex->nex_block;
        if (nex->nex_block) {
            nex->nex_block->pre_block = curr;
        }
        if (nex == last) {
            last = curr;
        }
        nex->valid = false;
    }
    block_len_index.insert(std::make_pair(curr->len_bytes, curr));
    return;
}

void MetaBlock::internal_extend(size_t at_least) {
    try {
        last->nex_block = std::make_shared<MemBlock>(true, std::countr_zero(std::bit_ceil(at_least)), *this);
        last->nex_block->pre_block = last;
        last = last->nex_block;
        block_len_index.insert(std::make_pair(last->len_bytes, last));
    } PUTILS_CATCH_THROW_GENERAL
}

MetaBlock::BlockHandle MetaBlock::try_assign(size_t target) {
    std::lock_guard<std::mutex> lock(list_lock);
    try {
        return internal_assign(target);
    } PUTILS_CATCH_THROW_GENERAL
}

MetaBlock::BlockHandle MetaBlock::extend_and_assign(size_t target) {
    std::lock_guard<std::mutex> lock(list_lock);
    try {
        size_t safe_target = (target + MemBlock::DEFAULT_ALIGNMENT - 1) / MemBlock::DEFAULT_ALIGNMENT * MemBlock::DEFAULT_ALIGNMENT;
        internal_extend(safe_target);
        return internal_assign(target);
    } PUTILS_CATCH_THROW_GENERAL
}

void release(MetaBlock::BlockHandle& handle) noexcept {
    if (!handle) {
        return;
    }
    auto& meta_block = handle->meta_block;
    std::lock_guard<std::mutex> lock(meta_block.list_lock);
    handle->free = true;
    meta_block.internal_compact(handle);
    handle = nullptr;
    return;
}


std::random_device MemoryPool::seed_generator;
size_t MemoryPool::num_lists_reservation = std::thread::hardware_concurrency() << 1;
size_t MemoryPool::initialization_block_size = 4194304;
std::atomic<bool> MemoryPool::initialized = false;
std::mutex MemoryPool::setting_lock;

MemoryPool::MemoryPool() {
    std::lock_guard<std::mutex> lock(setting_lock);
    MemoryPool::initialized.store(true, std::memory_order_release);
    try {
        for (size_t i = 0; i < MemoryPool::num_lists_reservation; i++) {
            list.push_back(std::make_unique<MetaBlock>(MemoryPool::initialization_block_size));
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
    size_t total = MemoryPool::num_lists_reservation * std::max<size_t>(MemoryPool::initialization_block_size, 1ull << MemBlock::DEFAULT_LOG_LEN_LOWER_BOUND);
    std::stringstream ss;
    ss << "(MemoryPool): initialization storage size: " << human(total);
    logger.add(ss.str());
    return true;
}

MemoryPool& MemoryPool::get_global_memorypool() noexcept {
    static MemoryPool memorypool_instance;
    return memorypool_instance;
}

MemoryPool::BlockHandle MemoryPool::allocate(size_t target) {
    thread_local std::mt19937 gen(MemoryPool::seed_generator());
    thread_local std::uniform_int_distribution<size_t> udist(0, MemoryPool::num_lists_reservation - 1);
    size_t idx = udist(gen);
    try {
        BlockHandle handle = list[idx]->try_assign(target);
        if (handle) {
            return handle;
        } else {
            return list[idx]->extend_and_assign(target);
        }
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
            if (!p->free) {
                view.bytes_in_use += p->len_bytes;
            }
        }
    }
    view.avg_block_size = view.bytes_total / std::max<size_t>(view.num_blocks, 1);
    view.usage_ratio = view.bytes_in_use * 1.0f / std::max<size_t>(view.bytes_total, 1);
    return view;
}

std::string MemoryPool::memory_distribution() noexcept {
    std::stringstream ss;
    for (size_t i = 0; i < list.size(); i++) {
        ss << "Block list #" << i + 1 << ": ";
        auto& head = list[i];
        std::lock_guard<std::mutex> lock(head->list_lock);
        for (BlockHandle p = head->first; p; p = p->nex_block) {
            ss << "[";
            if (p->header) {
                ss << " (head block) ";
            }
            if (p->free) {
                ss << " <free block> ";
            }
            ss << " size: " << human(p->len_bytes) << " ]";
            if (p->nex_block) {
                ss << "->";
            }
        }
        ss << std::endl;
    }
    return ss.str();
}

}
