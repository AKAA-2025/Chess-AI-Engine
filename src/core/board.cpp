#include "board.h"

Board::Board() {
    // White pieces: posx + (0 * 8)
    white_pawns   = RANK_2;
    white_rooks   = (1ULL << 0) | (1ULL << 7);
    white_knights = (1ULL << 1) | (1ULL << 6);
    white_bishops = (1ULL << 2) | (1ULL << 5);
    white_queens   = (1ULL << 3);
    white_kings    = (1ULL << 4);

    // Black pieces: posx + (7 * 8)
    black_pawns   = RANK_7;
    black_rooks   = (1ULL << 56) | (1ULL << 63);
    black_knights = (1ULL << 57) | (1ULL << 62);
    black_bishops = (1ULL << 58) | (1ULL << 61);
    black_queens   = (1ULL << 59);
    black_kings    = (1ULL << 60);
}

bool Board::isOccupied(uint64_t piece, int square) {
    if (square < 1 || square > 64) return false;

    return piece & (1ULL << square-1);
}

void Board::takePieceFrom(uint64_t piece, int square) {
    if (square < 1 || square > 64) return;

    piece &= ~(1ULL << square-1);
}

void Board::putPieceOn(uint64_t piece, int square) {
    if (square < 1 || square > 64) return;

    piece |= (1ULL << square-1);
}