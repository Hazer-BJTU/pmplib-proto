#include <vector>
#include "pmp/integer.h"

int main() {
    pmp::context context(1000, pmp::io::hex);
    std::vector<pmp::integer> arr;
    pmp::integer a("0", context), b("1", context);
    arr.push_back(a);
    arr.push_back(b);
    for (size_t i = 2; i < 10; i++) {
        arr.push_back(arr[i - 1] + arr[i - 2]);
    }
    pmp::integer c = arr[9], d = arr[6];
    arr.clear();
    context.generate_procedures();
    context.await_pipeline_accomplish();
    context.clean_up();
    std::cout << c << std::endl;
    return 0;
}