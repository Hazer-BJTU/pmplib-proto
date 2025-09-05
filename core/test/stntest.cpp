#include "StructuredNotation.hpp"

using namespace mpengine;

int main() {
    stn::beg_notation();
        stn::beg_field("field#1");
            stn::beg_list("list#1");
                stn::entry("string_element#1");
                stn::entry(1);
                stn::entry(true);
            stn::end_list();
            stn::beg_list("list#2");
                stn::entry("string_element#2");
                stn::entry(2);
                stn::entry(false);
            stn::end_list();
        stn::end_field();
        stn::beg_field("field#2");
            //empty
        stn::end_field();
        stn::beg_list("list#3");
            stn::beg_field();
                stn::entry("index", 1);
                stn::entry("label", "label#1");
            stn::end_field();
            stn::beg_field();
                stn::entry("index", 2);
                stn::entry("label", "label#2");
            stn::end_field();
            stn::beg_field();
                stn::entry("index", 3);
                stn::entry("label", "label#3");
            stn::end_field();
        stn::end_list();
    stn::end_notation(std::cout);
    return 0;
}
