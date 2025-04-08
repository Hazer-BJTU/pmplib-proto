#include "Real.h"

namespace rpc1k {

Real::Real() {
    p = std::make_shared<ConstantNode>();
}

Real::Real(std::string str) {
    p = std::make_shared<ConstantNode>(str);
}

Real::Real(const Real& r) {
    p = r.p;
}

Real& Real::operator = (const Real& r) {
    if (this != &r) {
        p = r.p;
    }
    return *this;
}

Real::~Real() {}

std::ostream& operator << (std::ostream& stream, const Real& r) {
    std::shared_ptr<ConstantNode> cp = std::dynamic_pointer_cast<ConstantNode>(r.p);
    if (cp == nullptr) {
        throw std::invalid_argument("[Error]: Not a constant node!");
    }
    std::shared_ptr<BasicNum> num_ptr(cp->getNum());
    stream << *num_ptr;
    return stream;
}

}
