#pragma once
#include "board.h"

class Chess {
public:
    Chess(Board board);

    bool white_to_move;

    uint64_t en_passant;

    void whiteCanCastleKS();
    void whiteCanCastleQS();
    void blackCanCastleKS();
    void blackCanCastleQS();

    bool isGameOver();

private:
    uint8_t castling_right;
};