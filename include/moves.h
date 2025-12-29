#pragma once
#include <string>

struct Move {
    int from;
    int to;
    std::string notation;
};

struct Move getMove(int f, int to, const std::string notation);