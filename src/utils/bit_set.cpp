#include "bit_set.h"

BitSet::BitSet() {
    BitsSetTable256[0] = 0; 
    for (int i = 0; i < 256; i++)
    { 
        BitsSetTable256[i] = (i & 1) + 
        BitsSetTable256[i / 2]; 
    };
}

int BitSet::countSetBits64(int x) {
    return (
        BitsSetTable256[x & 0xff] +
        BitsSetTable256[(x >> 8)  & 0xff] +
        BitsSetTable256[(x >> 16) & 0xff] +
        BitsSetTable256[(x >> 24) & 0xff] +
        BitsSetTable256[(x >> 32) & 0xff] +
        BitsSetTable256[(x >> 40) & 0xff] +
        BitsSetTable256[(x >> 48) & 0xff] +
        BitsSetTable256[(x >> 56) & 0xff]
    );
}