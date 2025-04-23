#include "BaseNum.h"

namespace rpc1k {

BaseNum::BaseNum(): sign(1) {
    auto& allocator = SegmentAllocator::get_global_allocator();
    allocator.assign(data);
}

BaseNum::~BaseNum() {
    auto& allocator = SegmentAllocator::get_global_allocator();
    allocator.free(data);
}

BaseNum::BaseNum(const BaseNum& num) {
    auto& allocator = SegmentAllocator::get_global_allocator();
    allocator.assign(data);
    memcpy(data, num.data, LENGTH * sizeof(int64));
}

BaseNum& BaseNum::operator = (const BaseNum& num) {
    if (this == &num) {
        return *this;
    }
    auto& allocator = SegmentAllocator::get_global_allocator();
    allocator.assign(data);
    memcpy(data, num.data, LENGTH * sizeof(int64));
    return *this;
}

const int64& BaseNum::operator [] (int idx) const {
    if (idx < 0 || idx >= LENGTH) {
        FREELOG("Index access out of range!", errlevel::WARNING);
        return data[0];
    }
    return data[idx];
}

bool BaseNum::get_sign() const {
    return sign;
}

}
