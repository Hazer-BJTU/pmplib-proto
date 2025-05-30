#include "Log.h"
#include <random>

using namespace std;

random_device rd;
mt19937 gen(rd());
uniform_int_distribution<int> udist(1, 100);

void function() {
    int random_number = udist(gen);
    if (random_number <= 25) {
        throw invalid_argument("Hello world!");
    } else {
        try {
            function();
        } CATCH_THROW_GENERAL;
    }
    return;
}

int main() {
    try {
        function();
    } catch(putils::GeneralException& e) {
        cout << e.what() << endl;
    }
    return 0;
}
