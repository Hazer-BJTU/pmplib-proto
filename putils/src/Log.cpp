#include <Log.h>

namespace putils {

GeneralException::GeneralException(const std::string& str) {
    ss << str;
}

const char* GeneralException::what() const noexcept {
    return ss.str().c_str();
}

}
