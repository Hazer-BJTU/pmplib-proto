#pragma once

#include "Prememory.h"
#include <cstdlib>
#include <cstring>

namespace rpc1k {

using int64 = unsigned long long;

inline constexpr size_t LENGTH = SEGMENT_SIZE / sizeof(int64);
inline constexpr int BASE = 1e8;
inline constexpr int LGBASE = 8;
inline constexpr int ZERO = LENGTH / 2 - 1;

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

class BinaryOperationGraphNode {
private:
    std::shared_ptr<BaseNum> out_domain, in_domain_A, in_domain_B;
    
};

}
