#pragma once

#include <iostream>

inline void printTest(const std::string &frontMessage, const bool param) {
    std::cout << frontMessage << " testIs: "  << (param ? "Success" : "Failed") << std::endl;
}

