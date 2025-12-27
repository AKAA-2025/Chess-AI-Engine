#pragma once
#include <cstdint>

class BitSet {
public:
    uint8_t BitsSetTable256[256];
    BitSet();

    int countSetBits64(int x);
};