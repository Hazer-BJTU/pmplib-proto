#include <vector>

#include "pmp/integer.h"

int main() {
    pmp::context context(1000, pmp::io::hex);
    std::vector<pmp::integer> integers;
    integers.emplace_back("0", context);
    integers.emplace_back("1", context);
    for (int i = 2; i < 10; i++) {
        integers.emplace_back(integers[i - 1] + integers[i - 2]);
    }
    context.export_graph_details("dag.json");
    return 0;
}