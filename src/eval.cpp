#include "eval.h"

float evaluate(Board board, BitSet bs) {
    if (board.isWhiteMated()) return 1000.0f;
    if (board.isBlackMated()) return -1000.0f;
    if (board.isDraw()) return 0;

    return _evaluateMaterial(board, bs);
}

static float _evaluateMaterial(Board board, BitSet bs) {
    return (
        bs.countSetBits64(board.positions[white_pawn]) * PAWN_MATERIAL +
        bs.countSetBits64(board.positions[white_pawn]) * KNIGHT_MATERIAL +
        bs.countSetBits64(board.positions[white_pawn]) * BISHOP_MATERIAL +
        bs.countSetBits64(board.positions[white_pawn]) * ROOK_MATERIAL +
        bs.countSetBits64(board.positions[white_pawn]) * QUEEN_MATERIAL
        -
        bs.countSetBits64(board.positions[white_pawn]) * PAWN_MATERIAL -
        bs.countSetBits64(board.positions[white_pawn]) * KNIGHT_MATERIAL -
        bs.countSetBits64(board.positions[white_pawn]) * BISHOP_MATERIAL -
        bs.countSetBits64(board.positions[white_pawn]) * ROOK_MATERIAL -
        bs.countSetBits64(board.positions[white_pawn]) * QUEEN_MATERIAL
    );
}

static float _evaluatePosition(Board board) {

}