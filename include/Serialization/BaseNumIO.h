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

/**
 * @brief Parses floating-point numbers into fixed-point numbers with storage, output, and serialization capabilities.
 *
 * The RealParser class converts floating-point numbers (including scientific notation) into fixed-point numbers
 * stored in BaseNum format (see: RPC-1K\include\Core\BaseNum.h). It supports:
 * - Parsing from strings (including scientific notation like "1.23e-4")
 * - Output as fixed-point string (std::string)
 * - Serialization/deserialization in both CSV and binary formats
 * 
 * Usage Examples:
 * @code
 *    RealParser parser;
 *    std::shared_ptr<BaseNum> num(std::make_shared<BaseNum>());
 *    std::ofstream file_out("serialization_test.csv");
 *
 *    parser(num, "123451234.3e-1");       // Parse from string
 *    parser(num, file_out);               // Serialize to CSV
 *    file_out.close();
 *    
 *    std::shared_ptr<BaseNum> num2(std::make_shared<BaseNum>());
 *    std::ifstream file_in("serialization_test.csv");
 *    
 *    parser(num2, file_in);               // Deserialize from CSV
 *    std::cout << "Read csv: " << parser(num2) << std::endl;
 *    file_in.close();
 *    
 *    file_out.open("serialization_test.bin");
 *    parser(num, "-2344424.123424E-3");  // Parse another value
 *    parser(num, file_out, ::rpc1k::io::binary);  // Serialize to binary
 *    file_out.close();
 *    
 *    file_in.open("serialization_test.bin");
 *    parser(num2, file_in, ::rpc1k::io::binary);  // Deserialize from binary
 *    std::cout << "Read bin: " << parser(num2) << std::endl;
 * @endcode
 * 
 * @note The class throws NumberFormatError for invalid input formats
 * @author Hazer
 * @date 2025/4/25
 */

class RealParser {
private:
    std::shared_ptr<BaseNum> dst;
    std::string src;
    void read();
    int str2int(std::string& str);
    int locate(int64& x, int power);
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
