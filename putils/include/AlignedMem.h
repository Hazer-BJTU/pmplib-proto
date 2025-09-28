#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstdint>

namespace putils {

struct addrlen {
    void* addr;
    size_t length;
};

addrlen aligned_alloc(size_t alignment, size_t length);

void aligned_free(const addrlen& addrlen_v);

}
