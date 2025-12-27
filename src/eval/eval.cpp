#include "eval.h"

float evaluate(Board board, BitSet bs) {
    if (board.isWhiteMated()) return 1000.0f;
    if (board.isBlackMated()) return -1000.0f;
    if (board.isDraw()) return 0;

    return _evaluateMaterial(board, bs);
}

static float _evaluateMaterial(Board board, BitSet bs) {
    return (
        bs.countSetBits64(board.white_pawns) * PAWN_MATERIAL +
        bs.countSetBits64(board.white_knights) * KNIGHT_MATERIAL +
        bs.countSetBits64(board.white_bishops) * BISHOP_MATERIAL +
        bs.countSetBits64(board.white_rooks) * ROOK_MATERIAL +
        bs.countSetBits64(board.white_queens) * QUEEN_MATERIAL
        -
        bs.countSetBits64(board.black_pawns) * PAWN_MATERIAL -
        bs.countSetBits64(board.black_knights) * KNIGHT_MATERIAL -
        bs.countSetBits64(board.black_bishops) * BISHOP_MATERIAL -
        bs.countSetBits64(board.black_rooks) * ROOK_MATERIAL -
        bs.countSetBits64(board.black_queens) * QUEEN_MATERIAL
    );
}

static float _evaluatePosition(Board board) {

}