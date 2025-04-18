#include "Prememory.h"

namespace rpc1k {

void* aligned_alloc(size_t alignment, size_t size) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        AUTOLOG("Alignment must be a power of 2.", errlevel::ERROR, ERROR_INVALID_ARGUMENT);
    }
    if (size % alignment != 0) {
        AUTOLOG("Total size must be a multiple of of alignment.", errlevel::ERROR, ERROR_INVALID_ARGUMENT);
    }
    void* ptr = nullptr;
    #ifdef _WIN32
        ptr = _aligned_malloc(size, alignment);
    #else
        ptr = std::aligned_alloc(alignment, size);
    #endif
    if (!ptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void aligned_free(void* ptr) {
    if (!ptr) {
        return;
    }
    #ifdef _WIN32
        _aligned_free(ptr);
    #else
        std::free(ptr);
    #endif
    return;
}

std::unique_ptr<SegmentAllocator> SegmentAllocator::segment_allocator_instance(std::make_unique<SegmentAllocator>());

SegmentAllocator& SegmentAllocator::get_global_allocator() {
    return *segment_allocator_instance;
}

SegmentAllocator::SegmentAllocator() {
    segments.reserve(VECTOR_RESERVATION);
    for (int i = 0; i < RESERVATION; i++) {
        auto& seg_ptr = segments.emplace_back(std::make_pair(nullptr, false));
        seg_ptr.first = reinterpret_cast<uint8_t*>(::rpc1k::aligned_alloc(ALIGNMENT, SEGMENT_SIZE));
        addr_2_idx.insert(std::make_pair(seg_ptr.first, i));
        free_segment_idx.push(i);
    }
}

SegmentAllocator::~SegmentAllocator() {
    for (auto& seg_ptr: segments) {
        ::rpc1k::aligned_free(reinterpret_cast<void*>(seg_ptr.first));
    }
}

int SegmentAllocator::expand() {
    int len = segments.size(), delta = (len + EXPAND_RATIO - 1) / EXPAND_RATIO;
    for (int i = 0; i < delta; i++) {
        auto& seg_ptr = segments.emplace_back(std::make_pair(nullptr, false));
        seg_ptr.first = reinterpret_cast<uint8_t*>(::rpc1k::aligned_alloc(ALIGNMENT, SEGMENT_SIZE));
        addr_2_idx.insert(std::make_pair(seg_ptr.first, len + i));
        free_segment_idx.push(len + i);
    }
    return delta;
}

uint8_t* SegmentAllocator::request() {
    std::lock_guard<std::mutex> lock(global_lock);
    if (free_segment_idx.empty()) {
        int delta = expand();
    }
    int target = free_segment_idx.front();
    free_segment_idx.pop();
    segments[target].second = true;
    return segments[target].first;
}

bool SegmentAllocator::release(uint8_t* target) {
    std::lock_guard<std::mutex> lock(global_lock);
    auto it = addr_2_idx.find(target);
    if (it == addr_2_idx.end() || !segments[it->second].second) {
        return false;
    }
    free_segment_idx.push(it->second);
    segments[it->second].second = false;
    return true;
}

int SegmentAllocator::compact() {
    std::lock_guard<std::mutex> lock(global_lock);
    while(!free_segment_idx.empty()) {
        free_segment_idx.pop();
    }
    addr_2_idx.clear();
    int i = 0;
    for (int j = 0; j < segments.size(); j++) {
        while(i < j && segments[i].second) {
            i++;
        }
        if (i < j && segments[j].second) {
            //We know that the jth segment must be in use while the ith segment is not.
            std::swap(segments[i], segments[j]);
        }
    }
    int k = segments.size() - 1, cnt = 0;
    while(k >= RESERVATION && !segments[k].second) {
        ::rpc1k::aligned_free(reinterpret_cast<void*>(segments[k].first));
        segments.pop_back();
        k--, cnt++;
    }
    for (int j = 0; j < segments.size(); j++) {
        addr_2_idx.insert(std::make_pair(segments[j].first, j));
        if (!segments[j].second) {
            free_segment_idx.push(j);
        }
    }
    return cnt;
}

}
