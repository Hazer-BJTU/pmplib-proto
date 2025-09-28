#include "AlignedMem.h"

#if defined(__linux__)

#include <unistd.h>
#include <sys/mman.h>

namespace putils {

addrlen aligned_alloc(size_t alignment, size_t length) {
    if (length == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return {nullptr, 0};
    }
    #if defined(PUTILS_DIRECT_MEMORY_MAPPER)
        static const size_t page_size = sysconf(_SC_PAGE_SIZE);
        if (alignment > page_size) {
            return {nullptr, 0};
        }
        size_t total_size = (length + page_size - 1) & ~(page_size - 1);
        void* base = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (base == MAP_FAILED) {
            return {nullptr, 0};
        }
        return {base, total_size};
    #else
        void* base = nullptr;
        size_t total_size = length;
        if (posix_memalign(&base, alignment, length) != 0) {
            return {nullptr, 0};
        }
        return {base, length};
    #endif
}

void aligned_free(const addrlen& addrlen_v) {
    if (addrlen_v.addr == nullptr) {
        return;
    }
    #if defined(PUTILS_DIRECT_MEMORY_MAPPER)
        munmap(addrlen_v.addr, addrlen_v.length);
    #else
        free(addrlen_v.addr);
    #endif
}

}

#elif defined(_WIN32)

#include <windows.h>
#include <malloc.h>

namespace putils {

addrlen aligned_alloc(size_t alignment, size_t length) {
    if (length == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return {nullptr, 0};
    }
    void* base = _aligned_malloc(length, alignment);
    if (base == nullptr) {
        return {nullptr, 0};
    }
    return {base, length};
}

void aligned_free(const addrlen& addrlen_v) {
    if (addrlen_v.addr == nullptr) {
        return;
    }
    _aligned_free(addrlen_v.addr);
    return;
}

}

#endif
