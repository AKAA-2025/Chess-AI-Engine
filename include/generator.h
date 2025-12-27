#pragma once

#include <vector>
#include <cstdint>
#include "board.h"
#include "moves.h"

class MoveGenerator {
public:
    MoveGenerator(Board board);
    std::vector<Move> generateAllMoves();

private:
    void _initAttackTables();
    uint64_t _computeKnightAttacks();
    
};