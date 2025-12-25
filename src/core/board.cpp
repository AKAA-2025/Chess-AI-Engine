#include "board.h"

Board::Board() {
    // White pieces: posx + (0 * 8)
    white_pawns   = RANK_2;
    white_rooks   = (1 << 0) | (1 << 7);
    white_knights = (1 << 1) | (1 << 6);
    white_bishops = (1 << 2) | (1 << 5);
    white_queen   = (1 << 3);
    white_king    = (1 << 4);

    // Black pieces: posx + (7 * 8)
    black_pawns   = RANK_7;
    black_rooks   = (1 << 56) | (1 << 63);
    black_knights = (1 << 57) | (1 << 62);
    black_bishops = (1 << 58) | (1 << 61);
    black_queen   = (1 << 59);
    black_king    = (1 << 60);

    _updateOccupancy();
}

void Board::_updateOccupancy() {
    white_occ = (
        white_pawns |
        white_rooks |
        white_knights |
        white_bishops |
        white_queen |
        white_king
    );

    black_occ = (
        black_pawns |
        black_rooks |
        black_knights |
        black_bishops |
        black_queen |
        black_king
    );

    occ = white_occ | black_occ;
}

bool Board::isOccupied(uint64_t piece, int square) {
    if (square < 1 || square > 64) return false;

    return piece & (1 << square-1);
}

void Board::takePieceFrom(uint64_t piece, int square) {
    if (square < 1 || square > 64) return;

    piece &= ~(1 << square-1);
}

void Board::putPieceOn(uint64_t piece, int square) {
    if (square < 1 || square > 64) return;

    piece |= (1 << square-1);
}