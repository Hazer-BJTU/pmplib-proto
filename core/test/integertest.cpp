#include <vector>

#include "pmp/integer.h"

int main() {
    std::vector<pmp::integer> integers; 
    {
        pmp::context context(1000, pmp::io::hex);
        for (int i = 0; i < 10; i++) {
            integers.emplace_back(context.make_integer("FFFF"));
        }
    }
    integers.front().get_context().export_graph_details("dag.json");
    return 0;
}