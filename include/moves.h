#pragma once
#include <string>

class Move {
public:
    int from;
    int to;
    std::string notation;

    Move(int f, int t, const std::string& n) : from(f), to(t), notation(n) {}
};