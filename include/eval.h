#pragma once
#include "board.h"
#include <cstdint>

namespace Eval {
    
    // Piece values in centipawns
    constexpr int PAWN_VALUE = 100;
    constexpr int KNIGHT_VALUE = 320;
    constexpr int BISHOP_VALUE = 330;
    constexpr int ROOK_VALUE = 500;
    constexpr int QUEEN_VALUE = 900;
    constexpr int KING_VALUE = 20000;
    
    // Piece-square tables (from white's perspective)
    // Values are in centipawns, indexed [square] where square is 0-63
    namespace Tables {
        extern const int pawn_table[64];
        extern const int knight_table[64];
        extern const int bishop_table[64];
        extern const int rook_table[64];
        extern const int queen_table[64];
        extern const int king_middlegame_table[64];
        extern const int king_endgame_table[64];
    }
    
    /**
     * Main evaluation worker class
     */
    class Worker {
    public:
        explicit Worker(Board* board);
        
        /**
         * Evaluate the current position
         * @return Score in centipawns from white's perspective
         *         Positive = white is better, Negative = black is better
         */
        int evaluate();
        
        /**
         * Quick material-only evaluation
         */
        int evaluateMaterial();
        
        /**
         * Evaluate material with piece-square tables
         */
        int evaluateWithPST();
        
        /**
         * Check if position is in endgame
         */
        bool isEndgame();
        
    private:
        Board* board;
        
        // Helper functions
        int getMaterialScore();
        int getPieceSquareScore();
        int getMobilityScore();
        int getPawnStructureScore();
        int getKingSafetyScore();
        
        // Get piece-square value for a piece at a square
        int getPSTValue(PieceType piece, int square);
        
        // Count total material on board (for endgame detection)
        int getTotalMaterial();
    };
    
    /**
     * Utility functions
     */
    namespace Utils {
        // Mirror square vertically (for black pieces)
        int mirrorSquare(int square);
        
        // Get piece value
        int getPieceValue(PieceType piece);
    }
}
