#include "FiniteStateMachine.hpp"

class MyAutomaton: public mpengine::Automaton {
public:
    double num;
    size_t exp;
    bool sign_num, sign_exp;
    //Define what you need...
    MyAutomaton(): num(.0), exp(0), sign_num(1), sign_exp(1) {
        add_node("S0");
        add_node("S1");
        add_node("S2");
        add_transition("S0", "S1", '+');
        add_transition("S0", "S1", '-', [this] { sign_num = 0; } /* action */ );
        //TO DO ...
        set_starting("S0");
    }
    ~MyAutomaton() override {};
};

int main() {

    return 0;
}
