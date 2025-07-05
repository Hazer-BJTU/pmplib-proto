#pragma once

#include <random>

#include "LockFreeQueue.hpp"
#include "GeneralException.h"
#include "RuntimeLog.h"

namespace putils {

inline std::random_device device_random_generator;
int get_random_number(int lower_bound, int upper_bound);

}
