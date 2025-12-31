#pragma once
#include <string>
#include "pieces.h"

enum MoveType {
    NORMAL,
    CAPTURE,
    EN_PASSANT,
    CASTLING,
    PROMOTION
};

struct Move {
    int from;           // 1-64 (1-based)
    int to;             // 1-64 (1-based)
    std::string notation;
    MoveType type;
    PieceType promotionPiece; // Only used if type == PROMOTION
    
    Move() : from(0), to(0), notation(""), type(NORMAL), promotionPiece(white_pawn) {}
    Move(int f, int t, std::string n, MoveType mt = NORMAL) 
        : from(f), to(t), notation(n), type(mt), promotionPiece(white_pawn) {}
};
