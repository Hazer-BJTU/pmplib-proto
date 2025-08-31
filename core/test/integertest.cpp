#include "pmp/integer.h"

int main() {
    pmp::context context(1000, pmp::io::hex);
    pmp::integer a("123400", context), b("-0", context), c("444551234", context);
    pmp::integer d = a, e = d, f("abab", c.get_context());
    pmp::integer g = context.make_integer("ffffffffff"), h = g.get_context().make_integer("-14405");
    std::cout << a << std::endl;
    std::cout << b << std::endl;
    std::cout << c << std::endl;
    std::cout << d << std::endl;
    std::cout << e << std::endl;
    std::cout << f << std::endl;
    std::cout << g << std::endl;
    std::cout << h << std::endl;
    context.export_graph_details("dag.json");
    return 0;
}