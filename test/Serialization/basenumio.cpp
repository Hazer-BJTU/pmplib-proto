#include "BaseNum.h"
#include "BaseNumIO.h"
#include <fstream>

using namespace rpc1k;

int main() {
    RealParser parser;
    std::shared_ptr<BaseNum> num(std::make_shared<BaseNum>());
    std::ofstream file_out("serialization_test.csv");

    parser(num, "123451234.3e-1");
    parser(num, file_out);
    file_out.close();
    
    std::shared_ptr<BaseNum> num2(std::make_shared<BaseNum>());
    std::ifstream file_in("serialization_test.csv");
    
    parser(num2, file_in);
    std::cout << "Read csv: " << parser(num2) << std::endl;
    file_in.close();
    
    file_out.open("serialization_test.bin");
    parser(num, "-2344424.123424E-3");
    parser(num, file_out, ::rpc1k::io::binary);
    file_out.close();
    
    file_in.open("serialization_test.bin");
    parser(num2, file_in, ::rpc1k::io::binary);
    std::cout << "Read bin: " << parser(num2) << std::endl;
    
    return 0;
}
