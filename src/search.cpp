#include "search.h"
#include "generator.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace Search {

// ============================================================================
// Worker Implementation
// ============================================================================

Worker::Worker(Board* board, Eval::Worker* evaluator) 
    : board(board), evaluator(evaluator), should_stop(false), max_time_ms(0) {
    std::memset(pv_table, 0, sizeof(pv_table));
    std::memset(pv_length, 0, sizeof(pv_length));
}

SearchResult Worker::search(const SearchConfig& config) {
    should_stop = false;
    stats.reset();
    search_start_time = std::chrono::steady_clock::now();
    max_time_ms = config.max_time_ms;
    
    SearchResult result;
    
    if (config.use_iterative_deepening) {
        result = iterativeDeepening(config.max_depth, config.max_time_ms);
    } else {
        result.best_move = searchRoot(config.max_depth);
        result.score = alphaBetaRecursive(config.max_depth, -INFINITY_SCORE, 
                                          INFINITY_SCORE, 0);
        result.depth = config.max_depth;
        result.stats = stats;
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.stats.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - search_start_time);
    
    return result;
}

Move Worker::searchRoot(int depth) {
    MoveGenerator::Worker moveGen(board);
    std::vector<Move> moves = moveGen.generateAllMoves();
    std::vector<Move> legalMoves = moveGen.filterLegalMoves(moves);
    
    if (legalMoves.empty()) {
        return Move(); // No legal moves
    }
    
    Move bestMove = legalMoves[0];
    int bestScore = -INFINITY_SCORE;
    
    // Save board state
    uint64_t oldPositions[16];
    board->copyPositions(oldPositions);
    uint8_t oldPackedInfo = board->getPackedInfo();
    
    for (const auto& move : legalMoves) {
        if (shouldStop()) break;
        
        // Make move (simplified - you need to implement proper makeMove)
        board->makeMove(move);
        
        int score = -alphaBetaRecursive(depth - 1, -INFINITY_SCORE, 
                                        INFINITY_SCORE, 1);
        
        // Unmake move
        board->restorePositions(oldPositions);
        board->setPackedInfo(oldPackedInfo);
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }
    
    return bestMove;
}

int Worker::alphaBetaRecursive(int depth, int alpha, int beta, int ply) {
    stats.nodes_searched++;
    
    if (shouldStop()) {
        return 0;
    }
    
    // Update selective depth
    if (ply > stats.selective_depth) {
        stats.selective_depth = ply;
    }
    
    // Initialize PV length
    pv_length[ply] = ply;
    
    // Check for draw
    if (isDraw()) {
        return 0;
    }
    
    // Leaf node - evaluate or do quiescence search
    if (depth <= 0) {
        stats.quiescence_nodes++;
        return quiescence(alpha, beta, ply);
    }
    
    // Generate and order moves
    MoveGenerator::Worker moveGen(board);
    std::vector<Move> moves = moveGen.generateAllMoves();
    std::vector<Move> legalMoves = moveGen.filterLegalMoves(moves);
    
    // Checkmate or stalemate
    if (legalMoves.empty()) {
        if (moveGen.isInCheck()) {
            return -MATE_SCORE + ply; // Checkmate
        } else {
            return 0; // Stalemate
        }
    }
    
    // Order moves for better pruning
    orderMoves(legalMoves, ply);
    
    // Save board state
    uint64_t oldPositions[16];
    board->copyPositions(oldPositions);
    uint8_t oldPackedInfo = board->getPackedInfo();
    
    int bestScore = -INFINITY_SCORE;
    
    // Search all moves
    for (const auto& move : legalMoves) {
        if (shouldStop()) break;
        
        // Make move
        board->makeMove(move);
        
        // Recursive search
        int score = -alphaBetaRecursive(depth - 1, -beta, -alpha, ply + 1);
        
        // Unmake move
        board->restorePositions(oldPositions);
        board->setPackedInfo(oldPackedInfo);
        
        // Update best score
        if (score > bestScore) {
            bestScore = score;
            
            // Update PV
            pv_table[ply][ply] = move;
            for (int i = ply + 1; i < pv_length[ply + 1]; i++) {
                pv_table[ply][i] = pv_table[ply + 1][i];
            }
            pv_length[ply] = pv_length[ply + 1];
        }
        
        // Alpha-beta pruning
        if (score >= beta) {
            return beta; // Beta cutoff
        }
        
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return bestScore;
}

int Worker::alphaBetaIterative(int depth, int alpha, int beta, int ply) {
    // Stack-based iterative version of alpha-beta
    // This is more complex but avoids deep recursion
    
    struct SearchNode {
        int depth;
        int alpha;
        int beta;
        int ply;
        int move_index;
        int best_score;
        std::vector<Move> legal_moves;
        uint64_t saved_positions[16];
        uint8_t saved_packed_info;
        bool moves_generated;
        
        SearchNode() : depth(0), alpha(0), beta(0), ply(0), move_index(0),
                      best_score(-INFINITY_SCORE), saved_packed_info(0),
                      moves_generated(false) {}
    };
    
    std::vector<SearchNode> stack;
    stack.reserve(MAX_PLY);
    
    // Initialize root node
    SearchNode root;
    root.depth = depth;
    root.alpha = alpha;
    root.beta = beta;
    root.ply = ply;
    root.moves_generated = false;
    stack.push_back(root);
    
    int final_score = 0;
    
    while (!stack.empty() && !shouldStop()) {
        SearchNode& node = stack.back();
        stats.nodes_searched++;
        
        // Update selective depth
        if (node.ply > stats.selective_depth) {
            stats.selective_depth = node.ply;
        }
        
        // Check for draw
        if (isDraw()) {
            stack.pop_back();
            if (!stack.empty()) {
                stack.back().best_score = std::max(stack.back().best_score, 0);
            } else {
                final_score = 0;
            }
            continue;
        }
        
        // Leaf node
        if (node.depth <= 0) {
            stats.quiescence_nodes++;
            int score = quiescence(node.alpha, node.beta, node.ply);
            stack.pop_back();
            
            if (!stack.empty()) {
                stack.back().best_score = std::max(stack.back().best_score, -score);
                stack.back().alpha = std::max(stack.back().alpha, -score);
            } else {
                final_score = score;
            }
            continue;
        }
        
        // Generate moves if not done yet
        if (!node.moves_generated) {
            MoveGenerator::Worker moveGen(board);
            std::vector<Move> moves = moveGen.generateAllMoves();
            node.legal_moves = moveGen.filterLegalMoves(moves);
            node.moves_generated = true;
            
            // Checkmate or stalemate
            if (node.legal_moves.empty()) {
                int score;
                if (moveGen.isInCheck()) {
                    score = -MATE_SCORE + node.ply;
                } else {
                    score = 0;
                }
                stack.pop_back();
                
                if (!stack.empty()) {
                    stack.back().best_score = std::max(stack.back().best_score, -score);
                } else {
                    final_score = score;
                }
                continue;
            }
            
            orderMoves(node.legal_moves, node.ply);
            board->copyPositions(node.saved_positions);
            node.saved_packed_info = board->getPackedInfo();
        }
        
        // Process next move
        if (node.move_index < node.legal_moves.size()) {
            Move move = node.legal_moves[node.move_index];
            node.move_index++;
            
            // Make move
            board->makeMove(move);
            
            // Create child node
            SearchNode child;
            child.depth = node.depth - 1;
            child.alpha = -node.beta;
            child.beta = -node.alpha;
            child.ply = node.ply + 1;
            child.moves_generated = false;
            
            stack.push_back(child);
        } else {
            // All moves processed
            int score = node.best_score;
            
            // Restore position
            board->restorePositions(node.saved_positions);
            board->setPackedInfo(node.saved_packed_info);
            
            stack.pop_back();
            
            if (!stack.empty()) {
                stack.back().best_score = std::max(stack.back().best_score, -score);
                
                // Alpha-beta pruning
                if (-score >= stack.back().beta) {
                    // Beta cutoff - skip remaining moves
                    stack.back().move_index = stack.back().legal_moves.size();
                    stack.back().best_score = stack.back().beta;
                }
                
                if (-score > stack.back().alpha) {
                    stack.back().alpha = -score;
                }
            } else {
                final_score = score;
            }
        }
    }
    
    return final_score;
}

int Worker::quiescence(int alpha, int beta, int ply) {
    stats.nodes_searched++;
    
    // Stand pat - evaluate current position
    int standPat = evaluator->evaluate();
    
    // Beta cutoff
    if (standPat >= beta) {
        return beta;
    }
    
    // Update alpha
    if (standPat > alpha) {
        alpha = standPat;
    }
    
    // Generate captures only
    MoveGenerator::Worker moveGen(board);
    std::vector<Move> captures = moveGen.generateCaptures();
    std::vector<Move> legalCaptures = moveGen.filterLegalMoves(captures);
    
    // Order captures
    orderMoves(legalCaptures, ply);
    
    // Save board state
    uint64_t oldPositions[16];
    board->copyPositions(oldPositions);
    uint8_t oldPackedInfo = board->getPackedInfo();
    
    // Search captures
    for (const auto& move : legalCaptures) {
        if (shouldStop()) break;
        
        // Make move
        board->makeMove(move);
        
        int score = -quiescence(-beta, -alpha, ply + 1);
        
        // Unmake move
        board->restorePositions(oldPositions);
        board->setPackedInfo(oldPackedInfo);
        
        // Beta cutoff
        if (score >= beta) {
            return beta;
        }
        
        // Update alpha
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return alpha;
}

SearchResult Worker::iterativeDeepening(int max_depth, int max_time_ms) {
    SearchResult result;
    
    for (int depth = 1; depth <= max_depth; depth++) {
        if (shouldStop()) break;
        
        Move bestMove = searchRoot(depth);
        int score = alphaBetaRecursive(depth, -INFINITY_SCORE, INFINITY_SCORE, 0);
        
        // Update result
        result.best_move = bestMove;
        result.score = score;
        result.depth = depth;
        result.stats = stats;
        
        // Extract PV
        result.principal_variation.clear();
        for (int i = 0; i < pv_length[0]; i++) {
            result.principal_variation.push_back(pv_table[0][i]);
        }
        
        // Print iteration info
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - search_start_time);
        
        std::cout << "info depth " << depth 
                  << " score " << Utils::scoreToString(score)
                  << " nodes " << stats.nodes_searched
                  << " nps " << Utils::calculateNPS(stats.nodes_searched, elapsed)
                  << " time " << elapsed.count()
                  << " pv ";
        
        for (const auto& move : result.principal_variation) {
            std::cout << move.notation << " ";
        }
        std::cout << std::endl;
        
        // Check for mate
        if (isMate(score)) {
            break;
        }
    }
    
    return result;
}

void Worker::orderMoves(std::vector<Move>& moves, int ply) {
    // Simple move ordering: captures first, then others
    std::sort(moves.begin(), moves.end(), 
              [this](const Move& a, const Move& b) {
                  return getMoveOrderScore(a) > getMoveOrderScore(b);
              });
}

int Worker::getMoveOrderScore(const Move& move) {
    int score = 0;
    
    // Check if it's a capture
    if (move.notation.find('x') != std::string::npos) {
        score += 1000;
        
        // MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
        // This is simplified; you'd need to get actual piece types
        score += 100;
    }
    
    // Check if it's from PV
    // (Would need PV tracking from previous iteration)
    
    // Check if it's a promotion
    if (move.notation.find('=') != std::string::npos) {
        score += 800;
    }
    
    return score;
}

bool Worker::shouldStop() {
    if (should_stop) return true;
    
    // Check time limit every 1024 nodes
    if ((stats.nodes_searched & 0x3FF) == 0) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - search_start_time);
        
        if (max_time_ms > 0 && elapsed.count() >= max_time_ms) {
            should_stop = true;
            return true;
        }
    }
    
    return false;
}

bool Worker::isDraw() {
    // Simplified draw detection
    // You should implement:
    // - Fifty-move rule
    // - Threefold repetition
    // - Insufficient material
    
    return false;
}

bool Worker::isMate(int score) {
    return std::abs(score) >= MATE_SCORE - MAX_PLY;
}

int Worker::mateDistance(int score) {
    if (score > 0) {
        return (MATE_SCORE - score + 1) / 2;
    } else {
        return -(MATE_SCORE + score) / 2;
    }
}

// ============================================================================
// Utils Implementation
// ============================================================================

namespace Utils {
    std::string scoreToString(int score) {
        std::ostringstream oss;
        
        if (std::abs(score) >= MATE_SCORE - MAX_PLY) {
            int distance = (score > 0) ? 
                (MATE_SCORE - score + 1) / 2 : 
                -(MATE_SCORE + score) / 2;
            oss << "mate " << distance;
        } else {
            oss << "cp " << score;
        }
        
        return oss.str();
    }
    
    std::string formatTime(std::chrono::milliseconds ms) {
        auto seconds = ms.count() / 1000;
        auto millis = ms.count() % 1000;
        
        std::ostringstream oss;
        oss << seconds << "." << std::setfill('0') << std::setw(3) << millis << "s";
        return oss.str();
    }
    
    uint64_t calculateNPS(uint64_t nodes, std::chrono::milliseconds time) {
        if (time.count() == 0) return 0;
        return (nodes * 1000) / time.count();
    }
}

} // namespace Search