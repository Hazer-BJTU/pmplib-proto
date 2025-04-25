#pragma once

#include "Log.h"
#include "BaseNum.h"
#include <iostream>
#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <sstream>
#include <climits>
#include <iomanip>
#include <fstream>

namespace rpc1k {

using int64 = unsigned long long;

static constexpr int DEFAULT_INPUT_BASE = 10;
static constexpr int MAX_SRC_LENGTH = LENGTH * LGBASE;

class NumberFormatError: public std::exception {
private:
    std::string msg;
public:
    explicit NumberFormatError(const std::string& msg);
    const char* what() const noexcept override;
};

enum class io {binary, csv};

class RealParser {
private:
    std::shared_ptr<BaseNum> dst;
    std::string src;
    void read();
    int str2int(std::string& str);
    int locate(int& x, int power);
    void write();
    void write_to_file(std::ofstream& stream, io mode);
    void read_from_file(std::ifstream& stream, io mode);
public:
    RealParser();
    ~RealParser();
    bool operator () (const std::shared_ptr<BaseNum>& num, const std::string& str);
    const std::string& operator () (const std::shared_ptr<BaseNum>& num);
    std::ofstream& operator () (const std::shared_ptr<BaseNum>& num, std::ofstream& stream, io mode=io::csv);
    std::ifstream& operator () (const std::shared_ptr<BaseNum>& num, std::ifstream& stream, io mode=io::csv);
};

}
