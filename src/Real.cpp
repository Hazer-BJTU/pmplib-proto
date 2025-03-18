#include "../include/Real.h"

namespace rpc1k {

Real::Real() {
    p = std::make_shared<BasicNum>();
}

Real::Real(const Real& r) {
    assert(this != &r);
    p = std::make_shared<BasicNum>(*r.p);
}

Real& Real::operator = (const Real& r) {
    assert(this != &r);
    p = std::make_shared<BasicNum>(*r.p);
    return *this;
}

Real::~Real() {}

}
