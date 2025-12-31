#include "generator.h"
#include <cstring>

namespace MoveGenerator {

// ============================================================================
// AttackTables Implementation
// ============================================================================

uint64_t AttackTables::knight_attacks[64];
uint64_t AttackTables::king_attacks[64];
uint64_t AttackTables::white_pawn_attacks[64];
uint64_t AttackTables::black_pawn_attacks[64];
bool AttackTables::initialized = false;

void AttackTables::initialize() {
    if (initialized) return;
    
    // Knight moves (L-shape: 2 squares in one direction, 1 in perpendicular)
    for (int sq = 0; sq < 64; sq++) {
        uint64_t attacks = 0ULL;
        int rank = sq / 8;
        int file = sq % 8;
        
        int knight_moves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
        for (auto& move : knight_moves) {
            int new_rank = rank + move[0];
            int new_file = file + move[1];
            if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
                attacks |= (1ULL << (new_rank * 8 + new_file));
            }
        }
        knight_attacks[sq] = attacks;
    }
    
    // King moves (one square in any direction)
    for (int sq = 0; sq < 64; sq++) {
        uint64_t attacks = 0ULL;
        int rank = sq / 8;
        int file = sq % 8;
        
        for (int dr = -1; dr <= 1; dr++) {
            for (int df = -1; df <= 1; df++) {
                if (dr == 0 && df == 0) continue;
                int new_rank = rank + dr;
                int new_file = file + df;
                if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
                    attacks |= (1ULL << (new_rank * 8 + new_file));
                }
            }
        }
        king_attacks[sq] = attacks;
    }
    
    // White pawn attacks (diagonal forward)
    for (int sq = 0; sq < 64; sq++) {
        uint64_t attacks = 0ULL;
        int rank = sq / 8;
        int file = sq % 8;
        
        if (rank < 7) {
            if (file > 0) attacks |= (1ULL << (sq + 7)); // Capture left
            if (file < 7) attacks |= (1ULL << (sq + 9)); // Capture right
        }
        white_pawn_attacks[sq] = attacks;
    }
    
    // Black pawn attacks (diagonal backward)
    for (int sq = 0; sq < 64; sq++) {
        uint64_t attacks = 0ULL;
        int rank = sq / 8;
        int file = sq % 8;
        
        if (rank > 0) {
            if (file > 0) attacks |= (1ULL << (sq - 9)); // Capture left
            if (file < 7) attacks |= (1ULL << (sq - 7)); // Capture right
        }
        black_pawn_attacks[sq] = attacks;
    }
    
    initialized = true;
}

uint64_t AttackTables::computeRookAttacks(int square, uint64_t blockers) {
    uint64_t attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    
    // North
    for (int r = rank + 1; r < 8; r++) {
        int sq = r * 8 + file;
        attacks |= (1ULL << sq);
        if (blockers & (1ULL << sq)) break;
    }
    // South
    for (int r = rank - 1; r >= 0; r--) {
        int sq = r * 8 + file;
        attacks |= (1ULL << sq);
        if (blockers & (1ULL << sq)) break;
    }
    // East
    for (int f = file + 1; f < 8; f++) {
        int sq = rank * 8 + f;
        attacks |= (1ULL << sq);
        if (blockers & (1ULL << sq)) break;
    }
    // West
    for (int f = file - 1; f >= 0; f--) {
        int sq = rank * 8 + f;
        attacks |= (1ULL << sq);
        if (blockers & (1ULL << sq)) break;
    }
    
    return attacks;
}

uint64_t AttackTables::computeBishopAttacks(int square, uint64_t blockers) {
    uint64_t attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    
    // NE
    for (int r = rank + 1, f = file + 1; r < 8 && f < 8; r++, f++) {
        int sq = r * 8 + f;
        attacks |= (1ULL << sq);
        if (blockers & (1ULL << sq)) break;
    }
    // NW
    for (int r = rank + 1, f = file - 1; r < 8 && f >= 0; r++, f--) {
        int sq = r * 8 + f;
        attacks |= (1ULL << sq);
        if (blockers & (1ULL << sq)) break;
    }
    // SE
    for (int r = rank - 1, f = file + 1; r >= 0 && f < 8; r--, f++) {
        int sq = r * 8 + f;
        attacks |= (1ULL << sq);
        if (blockers & (1ULL << sq)) break;
    }
    // SW
    for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--) {
        int sq = r * 8 + f;
        attacks |= (1ULL << sq);
        if (blockers & (1ULL << sq)) break;
    }
    
    return attacks;
}

uint64_t AttackTables::getRookAttacks(int square, uint64_t occupancy) {
    return computeRookAttacks(square, occupancy);
}

uint64_t AttackTables::getBishopAttacks(int square, uint64_t occupancy) {
    return computeBishopAttacks(square, occupancy);
}

uint64_t AttackTables::getQueenAttacks(int square, uint64_t occupancy) {
    return getRookAttacks(square, occupancy) | getBishopAttacks(square, occupancy);
}

uint64_t AttackTables::getKnightAttacks(int square) {
    return knight_attacks[square];
}

uint64_t AttackTables::getKingAttacks(int square) {
    return king_attacks[square];
}

uint64_t AttackTables::getWhitePawnAttacks(int square) {
    return white_pawn_attacks[square];
}

uint64_t AttackTables::getBlackPawnAttacks(int square) {
    return black_pawn_attacks[square];
}

// ============================================================================
// Utils Implementation
// ============================================================================

namespace Utils {
    int getLSB(uint64_t bb) {
        if (bb == 0) return -1;
        return __builtin_ctzll(bb);
    }
    
    int popLSB(uint64_t& bb) {
        int lsb = getLSB(bb);
        bb &= bb - 1; // Remove LSB
        return lsb;
    }
    
    int popCount(uint64_t bb) {
        return __builtin_popcountll(bb);
    }
    
    int getRank(int square) {
        return square / 8;
    }
    
    int getFile(int square) {
        return square % 8;
    }
}

// ============================================================================
// Worker Implementation
// ============================================================================

Worker::Worker(Board* board) : board(board) {
    AttackTables::initialize();
}

std::vector<Move> Worker::generateAllMoves() {
    std::vector<Move> moves;
    moves.reserve(128); // Average branching factor
    
    generatePawnMoves(moves);
    generateKnightMoves(moves);
    generateBishopMoves(moves);
    generateRookMoves(moves);
    generateQueenMoves(moves);
    generateKingMoves(moves);
    generateCastlingMoves(moves);
    
    return moves;
}

std::vector<Move> Worker::generateCaptures() {
    std::vector<Move> moves;
    moves.reserve(32);
    
    generatePawnMoves(moves, true);
    generateKnightMoves(moves, true);
    generateBishopMoves(moves, true);
    generateRookMoves(moves, true);
    generateQueenMoves(moves, true);
    generateKingMoves(moves, true);
    
    return moves;
}

void Worker::generatePawnMoves(std::vector<Move>& moves, bool capturesOnly) {
    bool isWhite = board->isWhiteTurn();
    uint64_t pawns = isWhite ? board->positions[white_pawn] : board->positions[black_pawn];
    uint64_t enemyPieces = isWhite ? board->positions[black_occ] : board->positions[white_occ];
    uint64_t empty = ~board->positions[occ];
    int direction = isWhite ? 8 : -8;
    int startRank = isWhite ? 1 : 6;
    int promoRank = isWhite ? 7 : 0;
    
    while (pawns) {
        int from = Utils::popLSB(pawns);
        int rank = Utils::getRank(from);
        
        // Single push
        if (!capturesOnly) {
            int to = from + direction;
            if (to >= 0 && to < 64 && (empty & (1ULL << to))) {
                int toRank = Utils::getRank(to);
                if (toRank == promoRank) {
                    addPawnPromotions(moves, from, to, false);
                } else {
                    moves.push_back(Move(from + 1, to + 1, 
                        squareToAlgebraic(from) + squareToAlgebraic(to)));
                }
                
                // Double push
                if (rank == startRank) {
                    int doubleTo = from + 2 * direction;
                    if (empty & (1ULL << doubleTo)) {
                        moves.push_back(Move(from + 1, doubleTo + 1, 
                            squareToAlgebraic(from) + squareToAlgebraic(doubleTo)));
                    }
                }
            }
        }
        
        // Captures
        uint64_t attacks = isWhite ? 
            AttackTables::getWhitePawnAttacks(from) : 
            AttackTables::getBlackPawnAttacks(from);
        uint64_t captures = attacks & enemyPieces;
        
        while (captures) {
            int to = Utils::popLSB(captures);
            int toRank = Utils::getRank(to);
            if (toRank == promoRank) {
                addPawnPromotions(moves, from, to, true);
            } else {
                Move m(from + 1, to + 1, squareToAlgebraic(from) + "x" + squareToAlgebraic(to), CAPTURE);
                moves.push_back(m);
            }
        }
        
        // En passant
        if (board->positions[en_passant]) {
            bool canEnPassant = (isWhite && rank == 4) || (!isWhite && rank == 3);
            if (canEnPassant) {
                uint64_t enPassantSquare = board->positions[en_passant];
                if (attacks & enPassantSquare) {
                    int to = Utils::getLSB(enPassantSquare);
                    Move m(from + 1, to + 1, 
                        squareToAlgebraic(from) + "x" + squareToAlgebraic(to) + "e.p.", EN_PASSANT);
                    moves.push_back(m);
                }
            }
        }
    }
}

void Worker::generateKnightMoves(std::vector<Move>& moves, bool capturesOnly) {
    bool isWhite = board->isWhiteTurn();
    uint64_t knights = isWhite ? board->positions[white_knight] : board->positions[black_knight];
    uint64_t friendlyPieces = isWhite ? board->positions[white_occ] : board->positions[black_occ];
    uint64_t enemyPieces = isWhite ? board->positions[black_occ] : board->positions[white_occ];
    
    while (knights) {
        int from = Utils::popLSB(knights);
        uint64_t attacks = AttackTables::getKnightAttacks(from);
        uint64_t targets = capturesOnly ? (attacks & enemyPieces) : (attacks & ~friendlyPieces);
        
        addMovesFromBitboard(moves, from, targets, "N");
    }
}

void Worker::generateBishopMoves(std::vector<Move>& moves, bool capturesOnly) {
    bool isWhite = board->isWhiteTurn();
    uint64_t bishops = isWhite ? board->positions[white_bishop] : board->positions[black_bishop];
    uint64_t friendlyPieces = isWhite ? board->positions[white_occ] : board->positions[black_occ];
    uint64_t enemyPieces = isWhite ? board->positions[black_occ] : board->positions[white_occ];
    
    while (bishops) {
        int from = Utils::popLSB(bishops);
        uint64_t attacks = AttackTables::getBishopAttacks(from, board->positions[occ]);
        uint64_t targets = capturesOnly ? (attacks & enemyPieces) : (attacks & ~friendlyPieces);
        
        addMovesFromBitboard(moves, from, targets, "B");
    }
}

void Worker::generateRookMoves(std::vector<Move>& moves, bool capturesOnly) {
    bool isWhite = board->isWhiteTurn();
    uint64_t rooks = isWhite ? board->positions[white_rook] : board->positions[black_rook];
    uint64_t friendlyPieces = isWhite ? board->positions[white_occ] : board->positions[black_occ];
    uint64_t enemyPieces = isWhite ? board->positions[black_occ] : board->positions[white_occ];
    
    while (rooks) {
        int from = Utils::popLSB(rooks);
        uint64_t attacks = AttackTables::getRookAttacks(from, board->positions[occ]);
        uint64_t targets = capturesOnly ? (attacks & enemyPieces) : (attacks & ~friendlyPieces);
        
        addMovesFromBitboard(moves, from, targets, "R");
    }
}

void Worker::generateQueenMoves(std::vector<Move>& moves, bool capturesOnly) {
    bool isWhite = board->isWhiteTurn();
    uint64_t queens = isWhite ? board->positions[white_queen] : board->positions[black_queen];
    uint64_t friendlyPieces = isWhite ? board->positions[white_occ] : board->positions[black_occ];
    uint64_t enemyPieces = isWhite ? board->positions[black_occ] : board->positions[white_occ];
    
    while (queens) {
        int from = Utils::popLSB(queens);
        uint64_t attacks = AttackTables::getQueenAttacks(from, board->positions[occ]);
        uint64_t targets = capturesOnly ? (attacks & enemyPieces) : (attacks & ~friendlyPieces);
        
        addMovesFromBitboard(moves, from, targets, "Q");
    }
}

void Worker::generateKingMoves(std::vector<Move>& moves, bool capturesOnly) {
    bool isWhite = board->isWhiteTurn();
    uint64_t king = isWhite ? board->positions[white_king] : board->positions[black_king];
    uint64_t friendlyPieces = isWhite ? board->positions[white_occ] : board->positions[black_occ];
    uint64_t enemyPieces = isWhite ? board->positions[black_occ] : board->positions[white_occ];
    
    if (king) {
        int from = Utils::getLSB(king);
        uint64_t attacks = AttackTables::getKingAttacks(from);
        uint64_t targets = capturesOnly ? (attacks & enemyPieces) : (attacks & ~friendlyPieces);
        
        addMovesFromBitboard(moves, from, targets, "K");
    }
}

void Worker::generateCastlingMoves(std::vector<Move>& moves) {
    bool isWhite = board->isWhiteTurn();
    uint64_t occupied = board->positions[occ];
    
    if (isWhite) {
        // Kingside castling
        if (board->whiteCanCastleKS()) {
            if (!(occupied & ((1ULL << 5) | (1ULL << 6)))) { // f1, g1 empty
                if (!isSquareAttacked(4, false) && !isSquareAttacked(5, false) && 
                    !isSquareAttacked(6, false)) {
                    Move m(5, 7, "O-O", CASTLING);  // e1 to g1
                    moves.push_back(m);
                }
            }
        }
        // Queenside castling
        if (board->whiteCanCastleQS()) {
            if (!(occupied & ((1ULL << 1) | (1ULL << 2) | (1ULL << 3)))) { // b1, c1, d1 empty
                if (!isSquareAttacked(4, false) && !isSquareAttacked(3, false) && 
                    !isSquareAttacked(2, false)) {
                    Move m(5, 3, "O-O-O", CASTLING);  // e1 to c1
                    moves.push_back(m);
                }
            }
        }
    } else {
        // Kingside castling
        if (board->blackCanCastleKS()) {
            if (!(occupied & ((1ULL << 61) | (1ULL << 62)))) { // f8, g8 empty
                if (!isSquareAttacked(60, true) && !isSquareAttacked(61, true) && 
                    !isSquareAttacked(62, true)) {
                    Move m(61, 63, "O-O", CASTLING);  // e8 to g8
                    moves.push_back(m);
                }
            }
        }
        // Queenside castling
        if (board->blackCanCastleQS()) {
            if (!(occupied & ((1ULL << 57) | (1ULL << 58) | (1ULL << 59)))) { // b8, c8, d8 empty
                if (!isSquareAttacked(60, true) && !isSquareAttacked(59, true) && 
                    !isSquareAttacked(58, true)) {
                    Move m(61, 59, "O-O-O", CASTLING);  // e8 to c8
                    moves.push_back(m);
                }
            }
        }
    }
}

void Worker::addMovesFromBitboard(std::vector<Move>& moves, int from, uint64_t targets, 
                                   const std::string& pieceSymbol) {
    uint64_t enemyOcc = board->isWhiteTurn() ? board->positions[black_occ] : board->positions[white_occ];
    
    while (targets) {
        int to = Utils::popLSB(targets);
        bool isCapture = (enemyOcc & (1ULL << to)) != 0;
        std::string notation = pieceSymbol + squareToAlgebraic(from) + 
                               (isCapture ? "x" : "") + squareToAlgebraic(to);
        Move m(from + 1, to + 1, notation, isCapture ? CAPTURE : NORMAL);
        moves.push_back(m);
    }
}

void Worker::addPawnPromotions(std::vector<Move>& moves, int from, int to, bool isCapture) {
    std::string base = squareToAlgebraic(from) + (isCapture ? "x" : "") + squareToAlgebraic(to);
    bool isWhite = board->isWhiteTurn();
    
    Move queenPromo(from + 1, to + 1, base + "=Q", PROMOTION);
    queenPromo.promotionPiece = isWhite ? white_queen : black_queen;
    moves.push_back(queenPromo);
    
    Move rookPromo(from + 1, to + 1, base + "=R", PROMOTION);
    rookPromo.promotionPiece = isWhite ? white_rook : black_rook;
    moves.push_back(rookPromo);
    
    Move bishopPromo(from + 1, to + 1, base + "=B", PROMOTION);
    bishopPromo.promotionPiece = isWhite ? white_bishop : black_bishop;
    moves.push_back(bishopPromo);
    
    Move knightPromo(from + 1, to + 1, base + "=N", PROMOTION);
    knightPromo.promotionPiece = isWhite ? white_knight : black_knight;
    moves.push_back(knightPromo);
}

std::string Worker::squareToAlgebraic(int square) {
    if (square < 0 || square >= 64) return "??";
    char file = 'a' + (square % 8);
    char rank = '1' + (square / 8);
    return std::string() + file + rank;
}

bool Worker::isSquareAttacked(int square, bool byWhite) {
    uint64_t occupied = board->positions[occ];
    
    // Check pawn attacks
    if (byWhite) {
        if (AttackTables::getBlackPawnAttacks(square) & board->positions[white_pawn]) return true;
    } else {
        if (AttackTables::getWhitePawnAttacks(square) & board->positions[black_pawn]) return true;
    }
    
    // Check knight attacks
    uint64_t knights = byWhite ? board->positions[white_knight] : board->positions[black_knight];
    if (AttackTables::getKnightAttacks(square) & knights) return true;
    
    // Check king attacks
    uint64_t king = byWhite ? board->positions[white_king] : board->positions[black_king];
    if (AttackTables::getKingAttacks(square) & king) return true;
    
    // Check bishop/queen attacks
    uint64_t bishops = byWhite ? 
        (board->positions[white_bishop] | board->positions[white_queen]) :
        (board->positions[black_bishop] | board->positions[black_queen]);
    if (AttackTables::getBishopAttacks(square, occupied) & bishops) return true;
    
    // Check rook/queen attacks
    uint64_t rooks = byWhite ? 
        (board->positions[white_rook] | board->positions[white_queen]) :
        (board->positions[black_rook] | board->positions[black_queen]);
    if (AttackTables::getRookAttacks(square, occupied) & rooks) return true;
    
    return false;
}

bool Worker::isInCheck() {
    bool isWhite = board->isWhiteTurn();
    uint64_t king = isWhite ? board->positions[white_king] : board->positions[black_king];
    if (!king) return false;
    
    int kingSquare = Utils::getLSB(king);
    return isSquareAttacked(kingSquare, !isWhite);
}

bool Worker::isPseudoLegal(const Move& move) {
    std::vector<Move> allMoves = generateAllMoves();
    for (const auto& m : allMoves) {
        if (m.from == move.from && m.to == move.to) return true;
    }
    return false;
}

bool Worker::isLegal(const Move& move) {
    if (!isPseudoLegal(move)) return false;
    
    // Make the move
    if (!board->makeMove(move)) return false;
    
    // Check if the move leaves the king in check
    // Note: After makeMove, it's the opponent's turn, so we check if our king is attacked
    board->toogleTurn();  // Toggle back to check our king
    bool legal = !isInCheck();
    board->toogleTurn();  // Toggle back to opponent's turn
    
    // Unmake the move
    board->unmakeMove();
    
    return legal;
}

std::vector<Move> Worker::filterLegalMoves(const std::vector<Move>& pseudoMoves) {
    std::vector<Move> legalMoves;
    legalMoves.reserve(pseudoMoves.size());
    
    for (const auto& move : pseudoMoves) {
        // Make the move
        if (!board->makeMove(move)) continue;
        
        // Check if the move leaves the king in check
        board->toogleTurn();  // Toggle back to check our king
        bool legal = !isInCheck();
        board->toogleTurn();  // Toggle back
        
        // Unmake the move
        board->unmakeMove();
        
        if (legal) {
            legalMoves.push_back(move);
        }
    }
    
    return legalMoves;
}

} // namespace MoveGenerator
