#include "GeneralException.h"

using namespace std;

int add(int a, int b) {
    throw GENERAL_EXCEPTION("Hello world!", "no exception");
    return a + b;
}

void fun1() {
    try {
        add(1, 2);
    } CATCH_THROW_GENERAL
    return;
}

void fun2(const string& str) {
    try {
        fun1();
    } CATCH_THROW_GENERAL
}

int main() {
    try {
        fun2("nothing");
    } catch(exception& e) {
        cout << e.what() << endl;
    }
    return 0;
}
