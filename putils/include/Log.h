#pragma once

#include <exception>
#include <iostream>
#include <sstream>
#include <string>

namespace putils {

class GeneralException: public std::exception {
private:
    std::stringstream ss;
public:
    explicit GeneralException(const std::string& str);
    const char* what() const noexcept override;
};

}
