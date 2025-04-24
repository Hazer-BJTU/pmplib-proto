#include "BaseNumIO.h"

namespace rpc1k {

NumberFormatError::NumberFormatError(const std::string& msg) {
    this->msg = msg;
}

const char* NumberFormatError::what() const noexcept {
    return msg.c_str();
}

void RealParser::read() {
    //Clear the number value.
    memset(dst->data, 0, LENGTH * sizeof(int64));
    //Check if the input string is empty.
    if (src.length() == 0) {
        throw NumberFormatError("Empty input!");
    } else if (src.length() > MAX_SRC_LENGTH) {
        throw NumberFormatError("String input too long!");
    }
    //Check if the characters are all valid.
    std::string validc("0123456789+-eE.");
    std::for_each(src.begin(), src.end(), [&validc](auto& x) {
        if (validc.find(x) == std::string::npos) {
            throw NumberFormatError("Invalid character!");
        }
        if (x == 'E') {
            //Convert all characters into lowercase letters.
            x = 'e';
        }
    });
    //Handle the sign.
    if (src[0] == '-') {
        dst->sign = 0;
        src = src.substr(1);
    } else if (src[0] == '+') {
        dst->sign = 1;
        src = src.substr(1);
    } else {
        dst->sign = 1;
    }
    //Extract the exponent (if exits).
    int exp_pos = src.find('e'), exp = 0;
    if (exp_pos != std::string::npos) {
        std::string exp_part = src.substr(exp_pos + 1);
        src = src.substr(0, exp_pos);
        exp = str2int(exp_part);
    }
    //Extract the float point (if exits).
    int point_pos = src.find('.'), point = 0;
    if (point_pos != std::string::npos) {
        //Important! We need to calculate the real power of the rightmost number.
        point = point_pos - src.length() + 1;
        src.erase(point_pos, 1);
    }
    //Take into consideration both the exponent and float point.
    //The global bias of the power of the rightmost number should be:
    int bias = point + exp;
    for (int i = 0; i < src.length(); i++) {
        if (src[i] < '0' || src[i] > '9') {
            throw NumberFormatError("Invalid significand format!");
        }
        int power = src.length() - 1 - i + bias, v = src[i] - '0';
        int idx = locate(v, power);
        if (idx >= 0 && idx < LENGTH) {
            dst->data[idx] += v;
        }
    }
    return;
}

int RealParser::str2int(std::string& str) {
    int result = 0, sign = 1;
    if (str.length() == 0) {
        return 0;
    }
    if (str[0] == '-') {
        sign = -1;
        str = str.substr(1);
    } else if (str[0] == '+') {
        str = str.substr(1);
    }
    for (auto c: str) {
        if (c < '0' || c > '9') {
            throw NumberFormatError("Invalid exponent format!");
        }
        int v = c - '0';
        if (result > (INT_MAX - v) / 10) {
            throw NumberFormatError("Too big exponent for fixed point number!");
        } else {
            result = result * 10 + v;
        }
    }
    return result * sign;
}

int RealParser::locate(int& x, int power) {
    if (power >= 0) {
        int idx = power / LGBASE, remain = power % LGBASE;
        for (int i = 0; i < remain; i++) {
            //We need to adjust the digit to the right position between 00000000 ~ 99999999
            x *= DEFAULT_INPUT_BASE;
        }
        return ZERO + idx;
    } else {
        int idx = (-power - 1) / LGBASE + 1;
        int remain = LGBASE - 1 - ((-power - 1) % LGBASE);
        for (int i = 0; i < remain; i++) {
            //We need to adjust the digit to the right position between 00000000 ~ 99999999
            x *= DEFAULT_INPUT_BASE;
        }
        return ZERO - idx;
    }
}

void RealParser::write() noexcept {
    std::stringstream ss;
    int left = 0, right = LENGTH - 1;
    while(dst->data[left] == 0 && left + 1 < ZERO) {
        left++;
    }
    while(dst->data[right] == 0 && right > ZERO) {
        right--;
    }
    if (dst->sign == 0) {
        ss << '-';
    }
    for (int i = right; i >= left; i--) {
        if (i == right) {
            ss << dst->data[i];
        } else {
            ss << std::setw(LGBASE) << std::setfill('0') << dst->data[i];
        }
        if (i == ZERO) {
            ss << '.';
        }
    }
    src = ss.str();
    while(src.back() == '0') {
        src.pop_back();
    }
    if (src.back() == '.') {
        src.pop_back();
    }
    return;
}

RealParser::RealParser(): dst(nullptr), src("") {}

RealParser::~RealParser() {}

bool RealParser::operator () (const std::shared_ptr<BaseNum>& num, const std::string& str) {
    src = str;
    dst = num;
    try {
        read();
    } catch(NumberFormatError& e) {
        std::stringstream log_msg;
        log_msg << "(Initialize number format error): " << e.what() << " Initialize to zero by default.";
        FREELOG(log_msg.str(), errlevel::WARNING);
        return false;
    }
    return true;
}

const std::string& RealParser::operator () (const std::shared_ptr<BaseNum>& num) {
    src = "";
    dst = num;
    write();
    return src;
}

}
