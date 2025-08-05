#include "FiniteStateMachine.hpp"

int main() {
    mpengine::Automaton atm;
    atm.add_node("A");
    atm.add_node("B");
    atm.add_node("C", true);
    atm.add_transitions("A", "B", "abc");
    atm.set_starting("A");
    atm.step('x');
    return 0;
}
