#include <iostream>

#include "DirectMapper.h"

int main() {
    auto addrlen = putils::aligned_alloc(64, 128 * 4);
    int* arr = reinterpret_cast<int*>(addrlen.addr);
    for (size_t i = 0; i < 128; i++) {
        arr[i] = i;
    }
    for (size_t i = 0; i < 128; i++) {
        std::cout << "arr[" << i << "]=" << arr[i] << std::endl;
    }
    putils::aligned_free(addrlen);
    return 0;
}
