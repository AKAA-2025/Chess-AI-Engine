#include "search.h"
#include <algorithm>
#include <iostream>
#include <cstring>

namespace Search {

// ============================================================================
// MVV-LVA (Most Valuable Victim - Least Valuable Attacker) scores
// ============================================================================
static const int MVV_LVA[7][7] = {
    {0,  0,  0,  0,  0,  0,  0},
    {0, 15, 25, 35, 45, 55, 0},
    {0, 14, 24, 34, 44, 54, 0},
    {0, 13, 23, 33, 43, 53, 0},
    {0, 12, 22, 32, 42, 52, 0},
    {0, 11, 21, 31, 41, 51, 0},
    {0, 10, 20, 30, 40, 50, 0}
};

static int getPieceIndex(PieceType pt) {
    switch (pt) {
        case white_pawn: case black_pawn: return 1;
        case white_knight: case black_knight: return 2;
        case white_bishop: case black_bishop: return 3;
        case white_rook: case black_rook: return 4;
        case white_queen: case black_queen: return 5;
        case white_king: case black_king: return 6;
        default: return 0;
    }
}

// ============================================================================
// Worker Implementation
// ============================================================================

Worker::Worker(Board* board, MoveGenerator::Worker* moveGen, Eval::Worker* evaluator)
    : board(board), moveGen(moveGen), evaluator(evaluator), stopped(false), allocatedTime(-1) {
    clearTables();
}

void Worker::clearTables() {
    // Initialize killer moves with empty moves
    for (int i = 0; i < MAX_PLY; i++) {
        killerMoves[i][0] = Move();
        killerMoves[i][1] = Move();
    }
    
    std::memset(historyTable, 0, sizeof(historyTable));
    
    // Initialize PV table
    for (int i = 0; i < MAX_PLY; i++) {
        pvLength[i] = 0;
        for (int j = 0; j < MAX_PLY; j++) {
            pvTable[i][j] = Move();
        }
    }
}

SearchResult Worker::search(const SearchLimits& limits) {
    SearchResult result;
    currentLimits = limits;
    stopped = false;
    stats.reset();
    clearTables();
    
    searchStartTime = std::chrono::steady_clock::now();
    allocatedTime = calculateTimeAllocation();
    
    int maxDepth = limits.maxDepth;
    if (maxDepth <= 0 || maxDepth > MAX_PLY) maxDepth = MAX_PLY;
    
    // Iterative deepening
    for (int depth = 1; depth <= maxDepth && !stopped; depth++) {
        stats.depth = depth;
        pvLength[0] = 0;
        
        int score = alphaBeta(depth, -INFINITY_SCORE, INFINITY_SCORE, 0, true);
        
        if (stopped && depth > 1) {
            break;
        }
        
        // Update result
        if (pvLength[0] > 0) {
            result.bestMove = pvTable[0][0];
            result.score = score;
            result.depth = depth;
            result.pv = extractPV(depth);
            result.stats = stats;
            
            sendInfo(depth, score, result.pv);
        }
        
        // Check for mate
        if (score > MATE_THRESHOLD || score < -MATE_THRESHOLD) {
            break;
        }
        
        // Time management
        if (allocatedTime > 0 && stats.elapsedMs() > allocatedTime / 2) {
            break;
        }
    }
    
    return result;
}

int Worker::alphaBeta(int depth, int alpha, int beta, int ply, bool isPV) {
    if (shouldStop()) {
        stopped = true;
        return 0;
    }
    
    if (ply > stats.selDepth) {
        stats.selDepth = ply;
    }
    
    pvLength[ply] = ply;
    
    // Leaf node - go to quiescence
    if (depth <= 0) {
        return quiescence(alpha, beta, ply);
    }
    
    stats.nodes++;
    
    if (ply >= MAX_PLY - 1) {
        return evaluator->evaluate();
    }
    
    bool inCheck = moveGen->isInCheck();
    
    // Check extension
    if (inCheck && depth < MAX_PLY - ply) {
        depth++;
    }
    
    // Generate moves
    std::vector<Move> pseudoMoves = moveGen->generateAllMoves();
    std::vector<Move> legalMoves = moveGen->filterLegalMoves(pseudoMoves);
    
    if (legalMoves.empty()) {
        if (inCheck) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }
    
    // Score and sort moves
    std::vector<ScoredMove> scoredMoves;
    scoredMoves.reserve(legalMoves.size());
    for (const auto& move : legalMoves) {
        scoredMoves.push_back(ScoredMove(move, 0));
    }
    scoreMoves(scoredMoves, ply, Move());
    std::sort(scoredMoves.begin(), scoredMoves.end());
    
    int bestScore = -INFINITY_SCORE;
    int moveCount = 0;
    
    for (const auto& sm : scoredMoves) {
        const Move& move = sm.move;
        moveCount++;
        
        if (!board->makeMove(move)) {
            continue;
        }
        
        int score;
        
        if (moveCount == 1) {
            score = -alphaBeta(depth - 1, -beta, -alpha, ply + 1, isPV);
        } else {
            // PVS: null window search first
            score = -alphaBeta(depth - 1, -alpha - 1, -alpha, ply + 1, false);
            
            if (!stopped && score > alpha && score < beta) {
                score = -alphaBeta(depth - 1, -beta, -alpha, ply + 1, isPV);
            }
        }
        
        board->unmakeMove();
        
        if (stopped) {
            return bestScore > -INFINITY_SCORE ? bestScore : 0;
        }
        
        if (score > bestScore) {
            bestScore = score;
            
            if (score > alpha) {
                alpha = score;
                
                // Update PV
                pvTable[ply][ply] = move;
                for (int i = ply + 1; i < pvLength[ply + 1] && i < MAX_PLY; i++) {
                    pvTable[ply][i] = pvTable[ply + 1][i];
                }
                pvLength[ply] = pvLength[ply + 1];
                
                if (score >= beta) {
                    if (!isCapture(move)) {
                        updateKillers(move, ply);
                        updateHistory(move, depth);
                    }
                    return beta;
                }
            }
        }
    }
    
    return bestScore;
}

int Worker::quiescence(int alpha, int beta, int ply) {
    if (shouldStop()) {
        stopped = true;
        return 0;
    }
    
    stats.qnodes++;
    
    if (ply > stats.selDepth) {
        stats.selDepth = ply;
    }
    
    int standPat = evaluator->evaluate();
    
    if (standPat >= beta) {
        return beta;
    }
    
    if (alpha < standPat) {
        alpha = standPat;
    }
    
    // Generate only captures
    std::vector<Move> captures = moveGen->generateCaptures();
    std::vector<Move> legalCaptures = moveGen->filterLegalMoves(captures);
    
    // Sort captures by MVV-LVA
    std::vector<ScoredMove> scoredCaptures;
    for (const auto& move : legalCaptures) {
        int score = getMvvLvaScore(move);
        scoredCaptures.push_back(ScoredMove(move, score));
    }
    std::sort(scoredCaptures.begin(), scoredCaptures.end());
    
    for (const auto& sm : scoredCaptures) {
        const Move& move = sm.move;
        
        if (!board->makeMove(move)) {
            continue;
        }
        
        int score = -quiescence(-beta, -alpha, ply + 1);
        
        board->unmakeMove();
        
        if (stopped) {
            return alpha;
        }
        
        if (score >= beta) {
            return beta;
        }
        
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return alpha;
}

void Worker::scoreMoves(std::vector<ScoredMove>& moves, int ply, const Move& hashMove) {
    for (auto& sm : moves) {
        const Move& move = sm.move;
        
        // Hash move
        if (hashMove.from != 0 && move.from == hashMove.from && move.to == hashMove.to) {
            sm.score = 100000;
            continue;
        }
        
        // Captures
        if (isCapture(move)) {
            sm.score = 50000 + getMvvLvaScore(move);
            continue;
        }
        
        // Killer moves
        if (ply < MAX_PLY) {
            if (killerMoves[ply][0].from == move.from && killerMoves[ply][0].to == move.to) {
                sm.score = 40000;
                continue;
            }
            if (killerMoves[ply][1].from == move.from && killerMoves[ply][1].to == move.to) {
                sm.score = 39000;
                continue;
            }
        }
        
        // History
        int from = move.from - 1;
        int to = move.to - 1;
        if (from >= 0 && from < 64 && to >= 0 && to < 64) {
            sm.score = historyTable[from][to];
        }
    }
}

int Worker::getMvvLvaScore(const Move& move) {
    int capturedPiece = board->getPieceAt(move.to);
    if (capturedPiece == -1) return 0;
    
    int attackingPiece = board->getPieceAt(move.from);
    if (attackingPiece == -1) return 0;
    
    int victimIdx = getPieceIndex(static_cast<PieceType>(capturedPiece));
    int attackerIdx = getPieceIndex(static_cast<PieceType>(attackingPiece));
    
    return MVV_LVA[attackerIdx][victimIdx] * 100;
}

void Worker::updateKillers(const Move& move, int ply) {
    if (ply >= MAX_PLY) return;
    
    if (killerMoves[ply][0].from == move.from && killerMoves[ply][0].to == move.to) {
        return;
    }
    
    killerMoves[ply][1] = killerMoves[ply][0];
    killerMoves[ply][0] = move;
}

void Worker::updateHistory(const Move& move, int depth) {
    int from = move.from - 1;
    int to = move.to - 1;
    
    if (from >= 0 && from < 64 && to >= 0 && to < 64) {
        historyTable[from][to] += depth * depth;
        
        if (historyTable[from][to] > 30000) {
            for (int i = 0; i < 64; i++) {
                for (int j = 0; j < 64; j++) {
                    historyTable[i][j] /= 2;
                }
            }
        }
    }
}

bool Worker::shouldStop() {
    if (stopped) return true;
    
    if (currentLimits.maxNodes > 0 && stats.nodes >= currentLimits.maxNodes) {
        return true;
    }
    
    if (allocatedTime > 0 && (stats.nodes & 1023) == 0) {
        if (stats.elapsedMs() >= allocatedTime) {
            return true;
        }
    }
    
    return false;
}

int Worker::calculateTimeAllocation() {
    if (currentLimits.moveTime > 0) {
        return currentLimits.moveTime;
    }
    
    if (currentLimits.infinite) {
        return -1;
    }
    
    int timeRemaining = board->isWhiteTurn() ? currentLimits.wtime : currentLimits.btime;
    int increment = board->isWhiteTurn() ? currentLimits.winc : currentLimits.binc;
    
    if (timeRemaining <= 0) {
        return -1;
    }
    
    int movesToGo = (currentLimits.movestogo > 0) ? currentLimits.movestogo : 30;
    int baseTime = timeRemaining / movesToGo;
    int timeWithInc = baseTime + (increment * 3 / 4);
    int maxTime = timeRemaining / 4;
    
    return std::min(timeWithInc, maxTime);
}

std::vector<Move> Worker::extractPV(int depth) {
    std::vector<Move> pv;
    
    for (int i = 0; i < pvLength[0] && i < depth && i < MAX_PLY; i++) {
        if (pvTable[0][i].from == 0 && pvTable[0][i].to == 0) break;
        pv.push_back(pvTable[0][i]);
    }
    
    return pv;
}

bool Worker::isCapture(const Move& move) {
    return board->isOccupied(move.to);
}

void Worker::stop() {
    stopped = true;
}

void Worker::sendInfo(int depth, int score, const std::vector<Move>& pv) {
    long long elapsed = stats.elapsedMs();
    long long nodes = stats.nodes + stats.qnodes;
    long long nps = (elapsed > 0) ? (nodes * 1000 / elapsed) : 0;
    
    std::cout << "info";
    std::cout << " depth " << depth;
    std::cout << " seldepth " << stats.selDepth;
    
    if (score > MATE_THRESHOLD) {
        int mateIn = (MATE_SCORE - score + 1) / 2;
        std::cout << " score mate " << mateIn;
    } else if (score < -MATE_THRESHOLD) {
        int mateIn = (-MATE_SCORE - score) / 2;
        std::cout << " score mate " << mateIn;
    } else {
        std::cout << " score cp " << score;
    }
    
    std::cout << " nodes " << nodes;
    std::cout << " nps " << nps;
    std::cout << " time " << elapsed;
    
    if (!pv.empty()) {
        std::cout << " pv";
        for (const auto& move : pv) {
            int fromFile = (move.from - 1) % 8;
            int fromRank = (move.from - 1) / 8;
            int toFile = (move.to - 1) % 8;
            int toRank = (move.to - 1) / 8;
            
            std::cout << " " 
                      << static_cast<char>('a' + fromFile) 
                      << static_cast<char>('1' + fromRank)
                      << static_cast<char>('a' + toFile) 
                      << static_cast<char>('1' + toRank);
            
            if (move.type == PROMOTION) {
                switch (move.promotionPiece) {
                    case white_queen: case black_queen: std::cout << "q"; break;
                    case white_rook: case black_rook: std::cout << "r"; break;
                    case white_bishop: case black_bishop: std::cout << "b"; break;
                    case white_knight: case black_knight: std::cout << "n"; break;
                    default: break;
                }
            }
        }
    }
    
    std::cout << std::endl;
}

} // namespace Search
