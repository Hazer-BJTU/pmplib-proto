#include <vector>

#include "pmp/integer.h"
// #include "GlobalConfig.h"

int main() {
    pmp::context context(1000, pmp::io::hex);
    pmp::integer A("FFFF", context);
    pmp::integer B = A + A;
    pmp::integer C = B + A;
    pmp::integer D = B + B;
    context.export_graph_details("dag.json");
    return 0;
}