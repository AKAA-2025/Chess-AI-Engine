#include "board.h"

Board::Board() {
    // White positions: posx + (0 * 8)
    positions[white_pawn]   = RANK_2;
    positions[white_rook]   = (1ULL << 0) | (1ULL << 7);
    positions[white_knight] = (1ULL << 1) | (1ULL << 6);
    positions[white_bishop] = (1ULL << 2) | (1ULL << 5);
    positions[white_queen]   = (1ULL << 3);
    positions[white_king]   = (1ULL << 4);

    // Black positions: posx + (7 * 8)
    positions[black_pawn]   = RANK_7;
    positions[black_rook]   = (1ULL << 56) | (1ULL << 63);
    positions[black_knight] = (1ULL << 57) | (1ULL << 62);
    positions[black_bishop] = (1ULL << 58) | (1ULL << 61);
    positions[black_queen]   = (1ULL << 59);
    positions[black_king]    = (1ULL << 60);

    positions[en_passant] = 0x0;

    _packed_info = 0x1F;

    _updateOccupancy();
}

bool Board::isOccupied(int square) {
    if (square < 1 || square > 64) return false;

    return positions[occ] & (1ULL << square-1);
}

void Board::takePieceFrom(PieceType pieceType, int square) {
    if (square < 1 || square > 64) return;

    Board::positions[pieceType] &= ~(1ULL << square-1);
    Board::_updateOccupancy();
}

void Board::putPieceOn(PieceType pieceType, int square) {
    if (square < 1 || square > 64) return;

    Board::positions[pieceType] |= (1ULL << square-1);
    Board::_updateOccupancy();
}

void Board::_updateOccupancy() {
    positions[white_occ] = (
        positions[white_pawn] |
        positions[white_knight] |
        positions[white_bishop] |
        positions[white_rook] |
        positions[white_queen] |
        positions[white_king]
    );

    positions[black_occ] = (
        positions[black_pawn] |
        positions[black_knight] |
        positions[black_bishop] |
        positions[black_rook] |
        positions[black_queen] |
        positions[black_king]
    );

    positions[occ] = positions[white_occ] + positions[black_occ];
}

bool Board::isWhiteTurn() {
    return _packed_info & 1;
}

bool Board::whiteCanCastleKS() {
    return (_packed_info >> 1) & 1;
}

bool Board::whiteCanCastleQS() {
    return (_packed_info >> 2) & 1;
}

bool Board::blackCanCastleKS() {
    return (_packed_info >> 3) & 1;
}

bool Board::blackCanCastleQS() {
    return (_packed_info >> 4) & 1;
}