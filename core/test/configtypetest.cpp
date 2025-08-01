#include "GlobalConfig.h"

using namespace std;

int main() {
    while(true) {
        mpengine::ConfigType config;
        cin >> config;
        cout << "Type: " << config.get_type() << ", Value: " << config << endl;
    }
    return 0;
}