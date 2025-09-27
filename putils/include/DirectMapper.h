#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstdint>

namespace putils {

struct addrlen {
    void* addr;
    size_t length;
};

addrlen aligned_alloc_linux(size_t alignment, size_t length);

addrlen aligned_alloc_win(size_t alignment, size_t length);

void aligned_free_linux(const addrlen& addrlen_v);

void aligned_free_win(const addrlen& addrlen_v);

#if defined(__linux__)

inline addrlen (*aligned_alloc)(size_t alignment, size_t length) = &aligned_alloc_linux;

inline void (*aligned_free)(const addrlen& addrlen_v) = &aligned_free_linux;

#elif defined(_WIN32)

inline addrlen (*aligned_alloc)(size_t alignment, size_t length) = &aligned_alloc_win;

inline void (*aligned_free)(const addrlen& addrlen_v) = &aligned_free_win;

#endif

}
