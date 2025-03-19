#pragma once

#include "BasicNum.h"
#include "CompGraphNodes.h"
#include <memory>

namespace rpc1k {

class Real {
public:
    std::shared_ptr<BasicNum> p;
    Real();
    Real(std::string str);
    Real(const Real& r);
    Real& operator = (const Real& r);
    ~Real();
};

std::ostream& operator << (std::ostream& stream, const Real& r);

}
