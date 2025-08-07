#include <cmath>
#include "FiniteStateMachine.hpp"

class MyAutomaton: public mpengine::Automaton {
public:
    size_t exp;
    double num, fraction_digit;
    bool sign_num, sign_exp;
    MyAutomaton(): num(0), exp(0), fraction_digit(1), sign_num(1), sign_exp(1) {
        add_node("initial");
        add_node("num_sign");
        add_node("integer_part", true);
        add_node("fractional_part", true);
        add_node("exp_symbol");
        add_node("exp_sign");
        add_node("exp_part", true);
        add_transition("initial", "num_sign", '+');
        add_transition("initial", "num_sign", '-', [this] { sign_num = 0; });
        add_transition("initial", "integer_part", "0123456789", [this] { num = num * 10 + _current_event - '0'; });
        add_transition("initial", "fractional_part", '.', [this] { fraction_digit = 0.1; });
        add_transition("num_sign", "integer_part", "0123456789", [this] { num = num * 10 + _current_event - '0'; });
        add_transition("num_sign", "fractional_part", '.', [this] { fraction_digit = 0.1; });
        add_transition("integer_part", "integer_part", "0123456789", [this] { num = num * 10 + _current_event - '0'; });
        add_transition("integer_part", "fractional_part", '.', [this] { fraction_digit = 0.1; } );
        add_transition("integer_part", "exp_symbol", "Ee");
        add_transition("fractional_part", "fractional_part", "0123456789", [this] { num += (_current_event - '0') * fraction_digit; fraction_digit /= 10; } );
        add_transition("fractional_part", "exp_symbol", "Ee");
        add_transition("exp_symbol", "exp_sign", '+');
        add_transition("exp_symbol", "exp_sign", '-', [this] { sign_exp = 0; });
        add_transition("exp_symbol", "exp_part", "0123456789", [this] { exp = exp * 10 + _current_event - '0'; });
        add_transition("exp_sign", "exp_part", "0123456789", [this] { exp = exp * 10 + _current_event - '0'; });
        add_transition("exp_part", "exp_part", "0123456789", [this] { exp = exp * 10 + _current_event - '0'; });
        set_starting("initial");
    }
    ~MyAutomaton() override {};
    double process_double(const std::string& str) {
        reset();
        num = 0, exp = 0, fraction_digit = 1, sign_num = 1, sign_exp = 1;
        try {
            steps(str);
        } catch(...) {
            throw;
        }
        if (!accepted()) {
            throw std::runtime_error("rejected");
        }
        if (sign_exp) {
            num *= pow(10.0, exp);
        } else {
            num /= pow(10.0, exp);
        }
        if (!sign_num) {
            num = -num;
        }
        return num;
    }
};

int main() {
    std::string input;
    MyAutomaton my_automaton;
    while(std::cin >> input) {
        double num;
        try {
            num = my_automaton.process_double(input);
            std::cout << "Succeeded: " << num << std::endl;
        } catch(const std::exception& e) {
            std::cout << "Failed: " << e.what() << std::endl;
        }
    }
    return 0;
}
