#include "Prememory.h"
#include <random>
#include <cassert>

static constexpr int MAXN = 1024;
static constexpr int MAXM = 512;
using int64 = long long;

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> udist(0, 1);

    int64 *arr[MAXN], brr[MAXN][MAXM];
    auto& segment_allocator = ::rpc1k::SegmentAllocator::get_global_allocator();
    for (int i = 0; i < MAXN; i++) {
        segment_allocator.assign(arr[i]);
        for (int j = 0; j < MAXM; j++) {
            arr[i][j] = gen();
            brr[i][j] = arr[i][j];
        }
    }
    for (int i = 0; i < MAXN; i++) {
        if (udist(gen) == 1) {
            bool flag = segment_allocator.free(arr[i]);
            assert(flag == true);
        }
    }
    bool flag = true;
    int cnt = segment_allocator.compact();
    std::cout << cnt << " segments released." << std::endl;
    for (int i = 0; i < MAXN; i++) {
        if (arr[i]) {
            for (int j = 0; j < MAXM; j++) {
                assert(arr[i][j] == brr[i][j]);
            }
        }
    }
    for (int i = 0; i < MAXN; i++) {
        if (!arr[i]) {
            segment_allocator.assign(arr[i]);
            for (int j = 0; j < MAXM; j++) {
                arr[i][j] = gen();
            }
        }
    }
    cnt = segment_allocator.compact();
    std::cout << cnt << " segments released." << std::endl;
    std::cout << "Test complete." << std::endl;
    return 0;
}
