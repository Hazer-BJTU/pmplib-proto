#include <vector>

#include "pmp/integer.h"

#include "MemoryAllocator.h"

int main() {
    pmp::context context(1000, pmp::io::hex);
    std::vector<pmp::integer> arr;
    pmp::integer a("0", context), b("1", context);
    arr.push_back(a);
    arr.push_back(b);
    for (size_t i = 2; i < 10; i++) {
        arr.push_back(arr[i - 1] + arr[i - 2]);
    }
    context.generate_procedures();
    context.export_graph_details("/mnt/d/pmplib/others");
    return 0;
}