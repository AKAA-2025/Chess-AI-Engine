#include "uci.h"
#include <iostream>

int main() {
    // Disable buffering for immediate communication
    std::cout.setf(std::ios::unitbuf);
    std::cin.setf(std::ios::unitbuf);
    
    UCI::Protocol uci;
    uci.run();
    
    return 0;
}