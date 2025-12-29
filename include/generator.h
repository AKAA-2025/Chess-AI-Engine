#pragma once
#include "board.h"
#include "moves.h"
#include <vector>

namespace MoveGenerator {
    
    /**
     * Pre-computed attack tables for sliding pieces
     */
    class AttackTables {
    public:
        static void initialize();
        static uint64_t getRookAttacks(int square, uint64_t occupancy);
        static uint64_t getBishopAttacks(int square, uint64_t occupancy);
        static uint64_t getQueenAttacks(int square, uint64_t occupancy);
        static uint64_t getKnightAttacks(int square);
        static uint64_t getKingAttacks(int square);
        static uint64_t getWhitePawnAttacks(int square);
        static uint64_t getBlackPawnAttacks(int square);
        
    private:
        static uint64_t knight_attacks[64];
        static uint64_t king_attacks[64];
        static uint64_t white_pawn_attacks[64];
        static uint64_t black_pawn_attacks[64];
        static bool initialized;
        
        static uint64_t computeRookAttacks(int square, uint64_t blockers);
        static uint64_t computeBishopAttacks(int square, uint64_t blockers);
    };
    
    /**
     * Main move generation worker class
     */
    class Worker {
    public:
        explicit Worker(Board* board);
        
        // Generate all pseudo-legal moves
        std::vector<Move> generateAllMoves();
        
        // Generate only captures
        std::vector<Move> generateCaptures();
        
        // Check if a move is pseudo-legal
        bool isPseudoLegal(const Move& move);
        
        // Check if a move is fully legal (doesn't leave king in check)
        bool isLegal(const Move& move);
        
        // Check if a square is attacked by the given side
        bool isSquareAttacked(int square, bool byWhite);
        
        // Check if the current side to move is in check
        bool isInCheck();
        
        // Filter pseudo-legal moves to only legal moves
        std::vector<Move> filterLegalMoves(const std::vector<Move>& pseudoMoves);
        
    private:
        Board* board;
        
        // Generate moves for specific piece types
        void generatePawnMoves(std::vector<Move>& moves, bool capturesOnly = false);
        void generateKnightMoves(std::vector<Move>& moves, bool capturesOnly = false);
        void generateBishopMoves(std::vector<Move>& moves, bool capturesOnly = false);
        void generateRookMoves(std::vector<Move>& moves, bool capturesOnly = false);
        void generateQueenMoves(std::vector<Move>& moves, bool capturesOnly = false);
        void generateKingMoves(std::vector<Move>& moves, bool capturesOnly = false);
        void generateCastlingMoves(std::vector<Move>& moves);
        
        // Helper functions
        void addMovesFromBitboard(std::vector<Move>& moves, int from, uint64_t targets, 
                                   const std::string& pieceSymbol);
        void addPawnPromotions(std::vector<Move>& moves, int from, int to, bool isCapture);
        std::string squareToAlgebraic(int square);
        int algebraicToSquare(const std::string& algebraic);
        bool makeMove(const Move& move);
        void unmakeMove(const Move& move, uint64_t oldPositions[16], uint8_t oldPackedInfo);
    };
    
    /**
     * Utility functions
     */
    namespace Utils {
        // Get the least significant bit position
        int getLSB(uint64_t bb);
        
        // Pop the least significant bit and return its position
        int popLSB(uint64_t& bb);
        
        // Count the number of bits set
        int popCount(uint64_t bb);
        
        // Get rank (0-7) from square (0-63)
        int getRank(int square);
        
        // Get file (0-7) from square (0-63)
        int getFile(int square);
    }
}