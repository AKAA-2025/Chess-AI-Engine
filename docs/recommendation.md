# Chess Engine - Recommendations and Improvements

## Critical Missing Implementations

### 1. Board::makeMove() and Board::unmakeMove()

You **MUST** implement these for the search to work. Here's a skeleton:

```cpp
// In board.h
struct UndoInfo {
    uint64_t captured_piece_bb;
    PieceType captured_piece_type;
    uint64_t old_en_passant;
    uint8_t old_packed_info;
    Move move;
};

class Board {
    // ... existing code ...
    std::vector<UndoInfo> undo_stack;
    
public:
    bool makeMove(const Move& move);
    void unmakeMove();
};

// In board.cpp
bool Board::makeMove(const Move& move) {
    UndoInfo undo;
    undo.old_en_passant = positions[en_passant];
    undo.old_packed_info = _packed_info;
    undo.move = move;
    
    int from_sq = move.from - 1; // Convert to 0-based
    int to_sq = move.to - 1;
    
    // Find which piece is moving
    PieceType moving_piece = static_cast<PieceType>(getPieceAt(move.from));
    
    // Handle captures
    int captured = getPieceAt(move.to);
    if (captured != -1) {
        undo.captured_piece_type = static_cast<PieceType>(captured);
        undo.captured_piece_bb = positions[captured];
        positions[captured] &= ~(1ULL << to_sq);
    }
    
    // Move the piece
    positions[moving_piece] &= ~(1ULL << from_sq);
    positions[moving_piece] |= (1ULL << to_sq);
    
    // Handle special moves (castling, en passant, promotion)
    // ... implement these ...
    
    // Update en passant
    positions[en_passant] = 0;
    // If pawn double push, set en passant square
    
    // Update castling rights
    // ... implement these ...
    
    // Toggle turn
    _packed_info ^= 1;
    
    // Update occupancy
    _updateOccupancy();
    
    undo_stack.push_back(undo);
    return true;
}

void Board::unmakeMove() {
    if (undo_stack.empty()) return;
    
    UndoInfo undo = undo_stack.back();
    undo_stack.pop_back();
    
    // Restore state
    positions[en_passant] = undo.old_en_passant;
    _packed_info = undo.old_packed_info;
    
    // Unmove the piece
    int from_sq = undo.move.from - 1;
    int to_sq = undo.move.to - 1;
    
    PieceType moving_piece = static_cast<PieceType>(getPieceAt(undo.move.to));
    positions[moving_piece] &= ~(1ULL << to_sq);
    positions[moving_piece] |= (1ULL << from_sq);
    
    // Restore captured piece
    if (undo.captured_piece_type != -1) {
        positions[undo.captured_piece_type] = undo.captured_piece_bb;
    }
    
    _updateOccupancy();
}
```

## Recommended Enhancements

### 2. Transposition Table

Add hash table to store already-evaluated positions:

```cpp
// transposition_table.h
namespace Search {
    struct TTEntry {
        uint64_t zobrist_key;
        int score;
        int depth;
        int flag; // EXACT, LOWER_BOUND, UPPER_BOUND
        Move best_move;
    };
    
    class TranspositionTable {
        // Implementation here
    };
}
```

### 3. Move Ordering Improvements

Current move ordering is basic. Enhance with:

- **Killer moves**: Non-capture moves that caused cutoffs
- **History heuristic**: Track moves that caused cutoffs
- **MVV-LVA**: Most Valuable Victim - Least Valuable Attacker for captures
- **PV moves**: Moves from principal variation

```cpp
// In search.h
class Worker {
private:
    Move killer_moves[MAX_PLY][2];  // Two killers per ply
    int history_table[64][64];       // [from][to]
    
    void updateKillers(Move move, int ply);
    void updateHistory(Move move, int depth);
};
```

### 4. Zobrist Hashing

For position identification (required for transposition table and repetition detection):

```cpp
// zobrist.h
namespace Zobrist {
    void initialize();
    uint64_t hash(const Board* board);
    uint64_t updateHash(uint64_t current_hash, Move move, Board* board);
}
```

### 5. Null Move Pruning

Add to alpha-beta search:

```cpp
// In alphaBetaRecursive, before move loop:
if (depth >= 3 && !moveGen.isInCheck() && !is_null_move_search) {
    // Make null move (skip turn)
    board->toggleTurn();
    int nullScore = -alphaBetaRecursive(depth - 3, -beta, -beta + 1, ply + 1);
    board->toggleTurn();
    
    if (nullScore >= beta) {
        return beta; // Null move cutoff
    }
}
```

### 6. Late Move Reductions (LMR)

Reduce search depth for moves unlikely to be best:

```cpp
int new_depth = depth - 1;

// After first few moves, reduce depth for quiet moves
if (move_number > 3 && depth >= 3 && !is_capture && !in_check) {
    new_depth = depth - 2;  // Reduce by 1 extra ply
}

int score = -alphaBetaRecursive(new_depth, -beta, -alpha, ply + 1);

// If reduced search fails high, re-search at full depth
if (score > alpha && new_depth < depth - 1) {
    score = -alphaBetaRecursive(depth - 1, -beta, -alpha, ply + 1);
}
```

### 7. Aspiration Windows

For iterative deepening:

```cpp
SearchResult Worker::iterativeDeepening(int max_depth, int max_time_ms) {
    int alpha = -INFINITY_SCORE;
    int beta = INFINITY_SCORE;
    
    for (int depth = 1; depth <= max_depth; depth++) {
        if (depth >= 4) {
            // Use aspiration window
            alpha = last_score - 50;
            beta = last_score + 50;
        }
        
        int score = alphaBetaRecursive(depth, alpha, beta, 0);
        
        // If we fall outside window, re-search with full window
        if (score <= alpha || score >= beta) {
            score = alphaBetaRecursive(depth, -INFINITY_SCORE, INFINITY_SCORE, 0);
        }
        
        last_score = score;
        // ... rest of iteration ...
    }
}
```

## Performance Optimizations

### 8. Bitboard Optimizations

```cpp
// Use compiler intrinsics more consistently
inline int popcount(uint64_t bb) {
    return __builtin_popcountll(bb);
}

inline int lsb(uint64_t bb) {
    return __builtin_ctzll(bb);
}

inline int msb(uint64_t bb) {
    return 63 - __builtin_clzll(bb);
}

inline uint64_t pop_lsb(uint64_t& bb) {
    uint64_t lsb = bb & -bb;
    bb &= bb - 1;
    return lsb;
}
```

### 9. Magic Bitboards for Sliding Pieces

Replace on-the-fly attack generation with pre-computed magic bitboards for rooks and bishops. This is much faster but more complex to implement.

### 10. Evaluation Improvements

```cpp
// Add to Eval::Worker
private:
    int evaluatePawnStructure() {
        int score = 0;
        // Doubled pawns penalty
        // Isolated pawns penalty
        // Passed pawns bonus
        return score;
    }
    
    int evaluateKingSafety() {
        int score = 0;
        // Pawn shield bonus
        // Open files near king penalty
        // Attacking pieces near king
        return score;
    }
    
    int evaluateMobility() {
        // Count legal moves for each side
        // More mobility = better position
        return move_count_difference * 10;
    }
```

## Testing Recommendations

### 11. Perft Testing

Implement perft (performance test) to verify move generation:

```cpp
uint64_t perft(Board* board, int depth) {
    if (depth == 0) return 1;
    
    MoveGenerator::Worker gen(board);
    auto moves = gen.generateAllMoves();
    auto legal = gen.filterLegalMoves(moves);
    
    uint64_t nodes = 0;
    for (const auto& move : legal) {
        board->makeMove(move);
        nodes += perft(board, depth - 1);
        board->unmakeMove();
    }
    
    return nodes;
}
```

Compare against known positions:
- Starting position depth 6: 119,060,324 nodes
- Kiwipete position depth 5: 193,690,690 nodes

### 12. Test Suite

Create tests for:
- Mate in 1, 2, 3 positions
- Tactical puzzles
- Endgame positions
- Known games

## Architecture Improvements

### 13. UCI Protocol

Implement Universal Chess Interface for compatibility with GUIs:

```cpp
// uci.h
namespace UCI {
    void loop();
    void processCommand(const std::string& cmd);
}
```

### 14. Threading Support

For multi-core searching:

```cpp
class Search::Worker {
    std::vector<std::thread> search_threads;
    // Lazy SMP or Young Brothers Wait Concepts
};
```

## Priority Order

1. **CRITICAL**: Implement `makeMove()` and `unmakeMove()` - nothing works without this
2. **HIGH**: Add Zobrist hashing for repetition detection
3. **HIGH**: Implement transposition table
4. **MEDIUM**: Improve move ordering (killers, history)
5. **MEDIUM**: Add null move pruning
6. **MEDIUM**: Improve evaluation (pawn structure, king safety)
7. **LOW**: Magic bitboards
8. **LOW**: LMR and aspiration windows
9. **LOW**: UCI protocol
10. **LOW**: Multi-threading

## Debugging Tips

- Add `#define DEBUG_SEARCH` to print search tree
- Implement perft to verify move generation correctness
- Test evaluation symmetry (eval from white = -eval from black)
- Use known positions to test (e.g., "The Lucena Position", "Philidor Position")
- Compare against Stockfish at low depth

## Useful Resources

- Chess Programming Wiki: https://www.chessprogramming.org/
- Stockfish source code (reference implementation)
- Crafty source code (well-documented)
- Bruce Moreland's chess programming pages