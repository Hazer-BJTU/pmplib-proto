#include "pmp/integer.h"

int main() {
    pmp::context context(1000, pmp::io::hex);
    pmp::integer a = pmp::integer("1234", context), b("1234", context);
    pmp::integer c = std::move(a);
    pmp::integer d(c);
    context.export_graph_details("dag.json");
    return 0;
}