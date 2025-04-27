#pragma once

#include "Log.h"
#include <map>
#include <vector>
#include <memory>
#include <queue>
#include <algorithm>
#include <exception>

#ifdef _WIN32
    #include <malloc.h>
#else
    #include <cstdlib>
#endif

namespace rpc1k {

/**
 * @brief Cross-platform aligned memory allocation
 * 
 *  params: alignment Alignment requirement (must be a power of 2 and ≥ sizeof(void*))
 *          size Number of bytes to allocate
 *  return: Pointer to the aligned memory (must be freed with aligned_free)
 *  throws: std::bad_alloc() Throws an exception if allocation fails
 * 
 * @author: Hazer
 * @date: 2025/04/18
 */
void* aligned_alloc(size_t alignment, size_t size);
void aligned_free(void* ptr);

inline constexpr size_t ALIGNMENT = 64;
inline constexpr size_t SEGMENT_SIZE = 4096; //Each segment consists of 512 unsigned long long.
inline constexpr int RESERVATION = 256;
inline constexpr int EXPAND_RATIO = 2;
inline constexpr int VECTOR_RESERVATION = 1024;

/**
 * @brief Thread-safe memory allocator for fixed-size aligned memory segments.
 *
 * This class implements a singleton memory allocator that manages pre-allocated 
 * memory segments of fixed size (SEGMENT_SIZE) with strict alignment (ALIGNMENT).
 * It provides efficient memory reuse by maintaining a pool of available segments.
 *
 * Memory Management:
 * - Segments are allocated in chunks during initialization and expansion
 * - Released segments are recycled for future requests
 * - Compaction releases excess unused segments while maintaining RESERVATION
 *
 * @note All public methods are thread-safe.
 * @warning Users must not manually free the obtained segments - use free() instead.
 *
 * @author Hazer
 * @date 2025/04/18
 */
class SegmentAllocator {
private:
    static std::unique_ptr<SegmentAllocator> segment_allocator_instance;
    std::vector<std::pair<uint8_t*, bool>> segments;
    std::map<uint8_t*, int> addr_2_idx;
    std::queue<int> free_segment_idx;
    std::mutex global_lock;
    SegmentAllocator();
    ~SegmentAllocator();
    SegmentAllocator(const SegmentAllocator&) = delete;
    SegmentAllocator& operator = (const SegmentAllocator&) = delete;
    int expand();
    uint8_t* request();
    bool release(uint8_t* target);
    friend class std::default_delete<SegmentAllocator>;
public:
    static SegmentAllocator& get_global_allocator();
    int compact();
    template<class T>
    void assign(T& ptr);
    template<class T>
    bool free(T& ptr);
    template<class T>
    bool exchange(T& ptr);
};

template<class T>
void SegmentAllocator::assign(T& ptr) {
    ptr = reinterpret_cast<T>(request());
    return;
}

template<class T>
bool SegmentAllocator::free(T& ptr) {
    auto uint_ptr = reinterpret_cast<uint8_t*>(ptr);
    ptr = nullptr;
    return release(uint_ptr);
}

template<class T>
bool SegmentAllocator::exchange(T& ptr) {
    auto uint_ptr = reinterpret_cast<uint8_t*>(ptr);
    bool good_free = release(uint_ptr);
    ptr = reinterpret_cast<T>(request());
    return good_free;
}

}
