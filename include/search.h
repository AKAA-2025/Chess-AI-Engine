#pragma once
#include "board.h"
#include "moves.h"
#include "eval.h"
#include <vector>
#include <limits>
#include <chrono>

namespace Search {
    
    // Constants
    constexpr int INFINITY_SCORE = 32000;
    constexpr int MATE_SCORE = 31000;
    constexpr int MAX_PLY = 128;
    
    /**
     * Search statistics
     */
    struct SearchStats {
        uint64_t nodes_searched;
        uint64_t quiescence_nodes;
        int depth_reached;
        int selective_depth;
        std::chrono::milliseconds time_elapsed;
        
        SearchStats() : nodes_searched(0), quiescence_nodes(0), 
                       depth_reached(0), selective_depth(0), 
                       time_elapsed(0) {}
        
        void reset() {
            nodes_searched = 0;
            quiescence_nodes = 0;
            depth_reached = 0;
            selective_depth = 0;
            time_elapsed = std::chrono::milliseconds(0);
        }
    };
    
    /**
     * Search result
     */
    struct SearchResult {
        Move best_move;
        int score;
        int depth;
        SearchStats stats;
        std::vector<Move> principal_variation;
        
        SearchResult() : score(0), depth(0) {}
    };
    
    /**
     * Search configuration
     */
    struct SearchConfig {
        int max_depth;
        int max_time_ms;
        bool use_quiescence;
        bool use_iterative_deepening;
        bool verbose;
        
        SearchConfig() : max_depth(6), max_time_ms(10000), 
                        use_quiescence(true), 
                        use_iterative_deepening(true),
                        verbose(false) {}
    };
    
    /**
     * Main search worker class
     */
    class Worker {
    public:
        Worker(Board* board, Eval::Worker* evaluator);
        
        /**
         * Search for the best move
         * @param config Search configuration
         * @return Search result with best move and score
         */
        SearchResult search(const SearchConfig& config);
        
        /**
         * Recursive alpha-beta search
         * @param depth Depth to search
         * @param alpha Alpha bound
         * @param beta Beta bound
         * @param ply Current ply from root
         * @return Best score
         */
        int alphaBetaRecursive(int depth, int alpha, int beta, int ply);
        
        /**
         * Iterative version of alpha-beta search
         * @param depth Depth to search
         * @param alpha Alpha bound
         * @param beta Beta bound
         * @param ply Current ply from root
         * @return Best score
         */
        int alphaBetaIterative(int depth, int alpha, int beta, int ply);
        
        /**
         * Quiescence search (search only captures to avoid horizon effect)
         * @param alpha Alpha bound
         * @param beta Beta bound
         * @param ply Current ply from root
         * @return Best score
         */
        int quiescence(int alpha, int beta, int ply);
        
        /**
         * Root search that finds the best move
         * @param depth Depth to search
         * @return Best move
         */
        Move searchRoot(int depth);
        
        /**
         * Iterative deepening search
         * @param max_depth Maximum depth to search
         * @param max_time_ms Maximum time in milliseconds
         * @return Search result
         */
        SearchResult iterativeDeepening(int max_depth, int max_time_ms);
        
        /**
         * Get search statistics
         */
        const SearchStats& getStats() const { return stats; }
        
        /**
         * Reset search statistics
         */
        void resetStats() { stats.reset(); }
        
        /**
         * Stop the search
         */
        void stop() { should_stop = true; }
        
    private:
        Board* board;
        Eval::Worker* evaluator;
        SearchStats stats;
        bool should_stop;
        std::chrono::time_point<std::chrono::steady_clock> search_start_time;
        int max_time_ms;
        
        // Principal variation table
        Move pv_table[MAX_PLY][MAX_PLY];
        int pv_length[MAX_PLY];
        
        // Move ordering helpers
        void orderMoves(std::vector<Move>& moves, int ply);
        int getMoveOrderScore(const Move& move);
        
        // Check if we should stop searching
        bool shouldStop();
        
        // Check if position is a draw
        bool isDraw();
        
        // Detect checkmate and stalemate
        bool isMate(int score);
        int mateDistance(int score);
    };
    
    /**
     * Utility functions
     */
    namespace Utils {
        // Convert score to string with mate detection
        std::string scoreToString(int score);
        
        // Format time in human-readable format
        std::string formatTime(std::chrono::milliseconds ms);
        
        // Calculate nodes per second
        uint64_t calculateNPS(uint64_t nodes, std::chrono::milliseconds time);
    }
}