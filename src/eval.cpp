#include "eval.h"
#include "generator.h"

namespace Eval {

// ============================================================================
// Piece-Square Tables
// ============================================================================

namespace Tables {
    // Pawn table - encourage center control and advancement
    const int pawn_table[64] = {
         0,   0,   0,   0,   0,   0,   0,   0,
        50,  50,  50,  50,  50,  50,  50,  50,
        10,  10,  20,  30,  30,  20,  10,  10,
         5,   5,  10,  25,  25,  10,   5,   5,
         0,   0,   0,  20,  20,   0,   0,   0,
         5,  -5, -10,   0,   0, -10,  -5,   5,
         5,  10,  10, -20, -20,  10,  10,   5,
         0,   0,   0,   0,   0,   0,   0,   0
    };
    
    // Knight table - encourage central positions
    const int knight_table[64] = {
        -50, -40, -30, -30, -30, -30, -40, -50,
        -40, -20,   0,   0,   0,   0, -20, -40,
        -30,   0,  10,  15,  15,  10,   0, -30,
        -30,   5,  15,  20,  20,  15,   5, -30,
        -30,   0,  15,  20,  20,  15,   0, -30,
        -30,   5,  10,  15,  15,  10,   5, -30,
        -40, -20,   0,   5,   5,   0, -20, -40,
        -50, -40, -30, -30, -30, -30, -40, -50
    };
    
    // Bishop table - encourage long diagonals
    const int bishop_table[64] = {
        -20, -10, -10, -10, -10, -10, -10, -20,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -10,   0,   5,  10,  10,   5,   0, -10,
        -10,   5,   5,  10,  10,   5,   5, -10,
        -10,   0,  10,  10,  10,  10,   0, -10,
        -10,  10,  10,  10,  10,  10,  10, -10,
        -10,   5,   0,   0,   0,   0,   5, -10,
        -20, -10, -10, -10, -10, -10, -10, -20
    };
    
    // Rook table - encourage 7th rank and open files
    const int rook_table[64] = {
         0,   0,   0,   0,   0,   0,   0,   0,
         5,  10,  10,  10,  10,  10,  10,   5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
         0,   0,   0,   5,   5,   0,   0,   0
    };
    
    // Queen table - slight center preference
    const int queen_table[64] = {
        -20, -10, -10,  -5,  -5, -10, -10, -20,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -10,   0,   5,   5,   5,   5,   0, -10,
         -5,   0,   5,   5,   5,   5,   0,  -5,
          0,   0,   5,   5,   5,   5,   0,  -5,
        -10,   5,   5,   5,   5,   5,   0, -10,
        -10,   0,   5,   0,   0,   0,   0, -10,
        -20, -10, -10,  -5,  -5, -10, -10, -20
    };
    
    // King middlegame table - encourage castling and safety
    const int king_middlegame_table[64] = {
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -20, -30, -30, -40, -40, -30, -30, -20,
        -10, -20, -20, -20, -20, -20, -20, -10,
         20,  20,   0,   0,   0,   0,  20,  20,
         20,  30,  10,   0,   0,  10,  30,  20
    };
    
    // King endgame table - encourage centralization
    const int king_endgame_table[64] = {
        -50, -40, -30, -20, -20, -30, -40, -50,
        -30, -20, -10,   0,   0, -10, -20, -30,
        -30, -10,  20,  30,  30,  20, -10, -30,
        -30, -10,  30,  40,  40,  30, -10, -30,
        -30, -10,  30,  40,  40,  30, -10, -30,
        -30, -10,  20,  30,  30,  20, -10, -30,
        -30, -30,   0,   0,   0,   0, -30, -30,
        -50, -30, -30, -30, -30, -30, -30, -50
    };
}

// ============================================================================
// Worker Implementation
// ============================================================================

Worker::Worker(Board* board) : board(board) {}

int Worker::evaluate() {
    int score = 0;
    
    // Material score
    score += getMaterialScore();
    
    // Piece-square tables
    score += getPieceSquareScore();
    
    // Return from current side's perspective
    return board->isWhiteTurn() ? score : -score;
}

int Worker::evaluateMaterial() {
    int score = getMaterialScore();
    return board->isWhiteTurn() ? score : -score;
}

int Worker::evaluateWithPST() {
    int score = getMaterialScore() + getPieceSquareScore();
    return board->isWhiteTurn() ? score : -score;
}

int Worker::getMaterialScore() {
    int score = 0;
    
    // White pieces
    score += __builtin_popcountll(board->positions[white_pawn]) * PAWN_VALUE;
    score += __builtin_popcountll(board->positions[white_knight]) * KNIGHT_VALUE;
    score += __builtin_popcountll(board->positions[white_bishop]) * BISHOP_VALUE;
    score += __builtin_popcountll(board->positions[white_rook]) * ROOK_VALUE;
    score += __builtin_popcountll(board->positions[white_queen]) * QUEEN_VALUE;
    
    // Black pieces
    score -= __builtin_popcountll(board->positions[black_pawn]) * PAWN_VALUE;
    score -= __builtin_popcountll(board->positions[black_knight]) * KNIGHT_VALUE;
    score -= __builtin_popcountll(board->positions[black_bishop]) * BISHOP_VALUE;
    score -= __builtin_popcountll(board->positions[black_rook]) * ROOK_VALUE;
    score -= __builtin_popcountll(board->positions[black_queen]) * QUEEN_VALUE;
    
    return score;
}

int Worker::getPieceSquareScore() {
    int score = 0;
    
    // White pieces
    for (int pieceType = white_pawn; pieceType <= white_king; pieceType++) {
        uint64_t pieces = board->positions[pieceType];
        while (pieces) {
            int square = __builtin_ctzll(pieces);
            pieces &= pieces - 1; // Remove LSB
            
            score += getPSTValue(static_cast<PieceType>(pieceType), square);
        }
    }
    
    // Black pieces (need to mirror squares)
    for (int pieceType = black_pawn; pieceType <= black_king; pieceType++) {
        uint64_t pieces = board->positions[pieceType];
        while (pieces) {
            int square = __builtin_ctzll(pieces);
            pieces &= pieces - 1; // Remove LSB
            
            int mirroredSquare = Utils::mirrorSquare(square);
            score -= getPSTValue(static_cast<PieceType>(pieceType - 6), mirroredSquare);
        }
    }
    
    return score;
}

int Worker::getPSTValue(PieceType piece, int square) {
    bool endgame = isEndgame();
    
    switch (piece) {
        case white_pawn:
            return Tables::pawn_table[square];
        case white_knight:
            return Tables::knight_table[square];
        case white_bishop:
            return Tables::bishop_table[square];
        case white_rook:
            return Tables::rook_table[square];
        case white_queen:
            return Tables::queen_table[square];
        case white_king:
            return endgame ? Tables::king_endgame_table[square] : 
                           Tables::king_middlegame_table[square];
        default:
            return 0;
    }
}

bool Worker::isEndgame() {
    int material = getTotalMaterial();
    // Endgame if total material < 2 rooks + 2 bishops worth
    return material < (ROOK_VALUE * 4 + BISHOP_VALUE * 4);
}

int Worker::getTotalMaterial() {
    int material = 0;
    
    // Don't count pawns and kings for endgame detection
    material += __builtin_popcountll(board->positions[white_knight]) * KNIGHT_VALUE;
    material += __builtin_popcountll(board->positions[white_bishop]) * BISHOP_VALUE;
    material += __builtin_popcountll(board->positions[white_rook]) * ROOK_VALUE;
    material += __builtin_popcountll(board->positions[white_queen]) * QUEEN_VALUE;
    
    material += __builtin_popcountll(board->positions[black_knight]) * KNIGHT_VALUE;
    material += __builtin_popcountll(board->positions[black_bishop]) * BISHOP_VALUE;
    material += __builtin_popcountll(board->positions[black_rook]) * ROOK_VALUE;
    material += __builtin_popcountll(board->positions[black_queen]) * QUEEN_VALUE;
    
    return material;
}

int Worker::getMobilityScore() {
    // TODO: Implement mobility evaluation
    return 0;
}

int Worker::getPawnStructureScore() {
    // TODO: Implement pawn structure evaluation
    return 0;
}

int Worker::getKingSafetyScore() {
    // TODO: Implement king safety evaluation
    return 0;
}

// ============================================================================
// Utils Implementation
// ============================================================================

namespace Utils {
    int mirrorSquare(int square) {
        int rank = square / 8;
        int file = square % 8;
        return (7 - rank) * 8 + file;
    }
    
    int getPieceValue(PieceType piece) {
        switch (piece) {
            case white_pawn:
            case black_pawn:
                return PAWN_VALUE;
            case white_knight:
            case black_knight:
                return KNIGHT_VALUE;
            case white_bishop:
            case black_bishop:
                return BISHOP_VALUE;
            case white_rook:
            case black_rook:
                return ROOK_VALUE;
            case white_queen:
            case black_queen:
                return QUEEN_VALUE;
            case white_king:
            case black_king:
                return KING_VALUE;
            default:
                return 0;
        }
    }
}

} // namespace Eval
