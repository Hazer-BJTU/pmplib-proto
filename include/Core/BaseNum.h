#pragma once

#include "Prememory.h"
#include <cstdlib>
#include <cstring>

namespace rpc1k {

using int64 = unsigned long long;

static constexpr size_t LENGTH = SEGMENT_SIZE / sizeof(int64);
static constexpr int BASE = 1e8;
static constexpr int LGBASE = 8;
static constexpr int ZERO = LENGTH / 2 - 1;

class RealParser;

class BaseNum {
private:
    bool sign;
    int64* data;
    friend RealParser;
public:
    BaseNum();
    ~BaseNum();
    BaseNum(const BaseNum& num);
    BaseNum& operator = (const BaseNum& num);
    const int64& operator [] (int idx) const;
    bool get_sign() const;
};

}
