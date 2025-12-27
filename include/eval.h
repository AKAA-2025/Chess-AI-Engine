#pragma once

#include "board.h"
#include "bit_set.h"

#define PAWN_MATERIAL 1.0f
#define KNIGHT_MATERIAL 3.0f
#define BISHOP_MATERIAL 3.2f
#define ROOK_MATERIAL 5.0f
#define QUEEN_MATERIAL 9.0f
#define KING_MATERIAL 0f

float evaluate(Board board, BitSet bs);