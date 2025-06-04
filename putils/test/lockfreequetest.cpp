#include "LockFreeQueue.hpp"
#include <iostream>

using namespace std;

int main() {
    putils::LockFreeQueue<int> lfq(256);
    for (int i = 0; i < 10; i++) {
        lfq.try_enqueue(i);
    }
    shared_ptr<int> ptr;
    for (int i = 0; i < 10; i++) {
        lfq.try_pop(ptr);
        cout << *ptr << endl;
    }
    return 0;
}