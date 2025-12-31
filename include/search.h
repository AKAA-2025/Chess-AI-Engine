#pragma once

#include "board.h"
#include "generator.h"
#include "eval.h"
#include "moves.h"
#include <vector>
#include <chrono>
#include <atomic>
#include <limits>

namespace Search {

// Constants
constexpr int MAX_PLY = 64;
constexpr int MAX_MOVES = 256;
constexpr int INFINITY_SCORE = 100000;
constexpr int MATE_SCORE = 99000;
constexpr int MATE_THRESHOLD = 98000;

/**
 * Search statistics
 */
struct SearchStats {
    long long nodes;
    long long qnodes;          // Quiescence nodes
    int depth;
    int selDepth;              // Selective depth (max depth reached)
    int hashHits;
    std::chrono::steady_clock::time_point startTime;
    
    SearchStats() : nodes(0), qnodes(0), depth(0), selDepth(0), hashHits(0) {}
    
    void reset() {
        nodes = 0;
        qnodes = 0;
        depth = 0;
        selDepth = 0;
        hashHits = 0;
        startTime = std::chrono::steady_clock::now();
    }
    
    long long elapsedMs() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    }
};

/**
 * Search result structure
 */
struct SearchResult {
    Move bestMove;
    int score;
    int depth;
    std::vector<Move> pv;      // Principal variation
    SearchStats stats;
    
    SearchResult() : score(0), depth(0) {}
};

/**
 * Search limits and time management
 */
struct SearchLimits {
    int maxDepth;
    int moveTime;              // Time allocated for this move (ms)
    long long maxNodes;
    bool infinite;
    
    // Time controls
    int wtime;
    int btime;
    int winc;
    int binc;
    int movestogo;
    
    SearchLimits() 
        : maxDepth(MAX_PLY), moveTime(-1), maxNodes(-1), infinite(false),
        wtime(-1), btime(-1), winc(0), binc(0), movestogo(-1) {}
};

/**
 * Move ordering helper
 */
struct ScoredMove {
    Move move;
    int score;
    
    ScoredMove() : score(0) {}
    ScoredMove(const Move& m, int s) : move(m), score(s) {}
    
    bool operator<(const ScoredMove& other) const {
        return score > other.score;  // Higher score = better move
    }
};

/**
 * Main search worker class
 */
class Worker {
public:
    Worker(Board* board, MoveGenerator::Worker* moveGen, Eval::Worker* evaluator);
    
    /**
     * Start iterative deepening search
     * @param limits Search constraints
     * @return Best move and search info
     */
    SearchResult search(const SearchLimits& limits);
    
    /**
     * Stop the current search
     */
    void stop();
    
    /**
     * Check if search is stopped
     */
    bool isStopped() const { return stopped; }
    
    /**
     * Get current search stats
     */
    const SearchStats& getStats() const { return stats; }

private:
    Board* board;
    MoveGenerator::Worker* moveGen;
    Eval::Worker* evaluator;
    
    // Search state
    std::atomic<bool> stopped;
    SearchStats stats;
    SearchLimits currentLimits;
    
    // Move ordering data
    Move killerMoves[MAX_PLY][2];     // Two killer moves per ply
    int historyTable[64][64];          // [from][to] history heuristic
    Move pvTable[MAX_PLY][MAX_PLY];    // Principal variation table
    int pvLength[MAX_PLY];
    
    // Time management
    std::chrono::steady_clock::time_point searchStartTime;
    int allocatedTime;
    
    /**
     * Alpha-beta search with negamax framework
     */
    int alphaBeta(int depth, int alpha, int beta, int ply, bool isPV);
    
    /**
     * Quiescence search - search only captures
     */
    int quiescence(int alpha, int beta, int ply);
    
    /**
     * Score moves for move ordering
     */
    void scoreMoves(std::vector<ScoredMove>& moves, int ply, const Move& hashMove);
    
    /**
     * Get MVV-LVA score for captures
     */
    int getMvvLvaScore(const Move& move);
    
    /**
     * Update killer moves
     */
    void updateKillers(const Move& move, int ply);
    
    /**
     * Update history heuristic
     */
    void updateHistory(const Move& move, int depth);
    
    /**
     * Check time and other termination conditions
     */
    bool shouldStop();
    
    /**
     * Calculate time allocation for this move
     */
    int calculateTimeAllocation();
    
    /**
     * Extract principal variation from PV table
     */
    std::vector<Move> extractPV(int depth);
    
    /**
     * Check if a move is a capture
     */
    bool isCapture(const Move& move);
    
    /**
     * Clear search tables
     */
    void clearTables();
    
    /**
     * Send UCI info output
     */
    void sendInfo(int depth, int score, const std::vector<Move>& pv);
};

} // namespace Search
