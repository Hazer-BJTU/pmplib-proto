#pragma once

#include "BasicNum.h"
#include <memory>

namespace rpc1k {

class Real {
public:
    std::shared_ptr<BasicNum> p;
    Real();
    Real(const Real& r);
    Real& operator = (const Real& r);
    ~Real();
};

}
