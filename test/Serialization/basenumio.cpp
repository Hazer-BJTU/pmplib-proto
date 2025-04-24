#include "BaseNum.h"
#include "BaseNumIO.h"

using namespace rpc1k;

int main() {
    RealParser parser;
    while(true) {
        std::string input;
        std::cin >> input;
        std::shared_ptr number(std::make_shared<BaseNum>());
        parser(number, input);
        std::cout << "Result: " << parser(number) << std::endl;
    }
    return 0;
}
