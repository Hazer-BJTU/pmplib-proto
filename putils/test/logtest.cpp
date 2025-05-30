#include <iostream>
#include <sstream>
#include <execinfo.h>
#include <cstdlib>
#include <random>

using namespace std;

inline constexpr int MAX_DEPTH = 128;

random_device rd;
mt19937 gen(rd());
uniform_int_distribution udist(1, 100);

void print_stacktrace() {
    void* stack_addr[MAX_DEPTH];
    int stack_depth = backtrace(stack_addr, MAX_DEPTH);
    char **stack_strings = backtrace_symbols(stack_addr, stack_depth);
    for (int i = 0; i < stack_depth; i++) {
        cout << stack_strings[i] << endl;
    }
    free(stack_strings);
    exit(0);
}

void function() {
    int random_num = udist(gen);
    if (random_num <= 25) {
        print_stacktrace();
    } else {
        function();
    }
    return;
}

int main() {
    function();
    return 0;
}