#include "TaskHandler.h"

namespace putils {

int get_random_number(int lower_bound, int upper_bound) {
    if (lower_bound > upper_bound) {
        throw PUTILS_GENERAL_EXCEPTION("Invalid interval for random number!", "invalid argument");
    }
    thread_local std::mt19937 gen(device_random_generator());
    std::uniform_int_distribution<> udist(lower_bound, upper_bound);
    return udist(gen);
}

}
