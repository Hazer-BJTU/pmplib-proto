#include "Real.h"

namespace rpc1k {

Real::Real() {
    p = std::make_shared<BasicNum>();
}

Real::Real(std::string str) {
    p = std::make_shared<BasicNum>(str);
}

Real::Real(const Real& r) {
    p = std::make_shared<BasicNum>(*r.p);
}

Real& Real::operator = (const Real& r) {
    if (this != &r) {
        p = r.p;
    }
    return *this;
}

Real::~Real() {}

std::ostream& operator << (std::ostream& stream, const Real& r) {
    stream << *(r.p);
    return stream;
}

}
