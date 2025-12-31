#include "board.h"
#include <cstring>

Board::Board() {
    std::memset(positions, 0, sizeof(positions));

    // White positions: posx + (0 * 8)
    positions[white_pawn]   = RANK_2;
    positions[white_rook]   = (1ULL << 0) | (1ULL << 7);
    positions[white_knight] = (1ULL << 1) | (1ULL << 6);
    positions[white_bishop] = (1ULL << 2) | (1ULL << 5);
    positions[white_queen]   = (1ULL << 3);
    positions[white_king]   = (1ULL << 4);

    // Black positions: posx + (7 * 8)
    positions[black_pawn]   = RANK_7;
    positions[black_rook]   = (1ULL << 56) | (1ULL << 63);
    positions[black_knight] = (1ULL << 57) | (1ULL << 62);
    positions[black_bishop] = (1ULL << 58) | (1ULL << 61);
    positions[black_queen]   = (1ULL << 59);
    positions[black_king]    = (1ULL << 60);

    positions[en_passant] = 0x0;

    half_clock = 0U;

    _packed_info = 0x1F;  // White to move, all castling rights

    _updateOccupancy();
}

Board::Board(const char *fen) {
    std::memset(positions, 0, sizeof(positions));
    _packed_info = 0;

    char buf[256];
    std::strncpy(buf, fen, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *token = strtok(buf, " ");
    if (!token) return;
    _fenImportBoard(token);

    token = strtok(NULL, " ");
    if (!token) return;
    setTurn(token[0] == 'w');
    
    token = strtok(NULL, " ");
    if (!token) return;
    for (int i = 0; token[i] != '\0'; ++i) {
        switch (token[i]) {
            case 'K':
                setWhiteCanCastleKS(true);
                break;
            case 'Q':
                setWhiteCanCastleQS(true);
                break;
            case 'k':
                setBlackCanCastleKS(true);
                break;
            case 'q':
                setBlackCanCastleQS(true);
                break;
            default:
                break;
        }
    }

    token = strtok(NULL, " ");
    if (!token) return;
    if (token[0] != '-') {
        positions[en_passant] = 1ULL << ((token[0] - 'a') + (token[1] - '1') * 8);
    }

    token = strtok(NULL, " ");
    if (!token) return;
    half_clock = std::atoi(token);

    // Full move counter is ignored since it doesn't help the engine

    _updateOccupancy();
}

void Board::_fenImportBoard(const char* boardFen) {
    int rank = 7;
    int file = 0;

    for (int i = 0; boardFen[i] != '\0'; ++i) {
        char c = boardFen[i];

        if (c == '/') {
            rank--;
            file = 0;
            continue;
        }

        if (c >= '1' && c <= '8') {
            file += c - '0';
            continue;
        }

        int sq = rank * 8 + file;
        uint64_t bit = 1ULL << sq;

        switch (c) {
            case 'p': positions[black_pawn]   |= bit; break;
            case 'n': positions[black_knight] |= bit; break;
            case 'b': positions[black_bishop] |= bit; break;
            case 'r': positions[black_rook]   |= bit; break;
            case 'q': positions[black_queen]  |= bit; break;
            case 'k': positions[black_king]   |= bit; break;
            case 'P': positions[white_pawn]   |= bit; break;
            case 'N': positions[white_knight] |= bit; break;
            case 'B': positions[white_bishop] |= bit; break;
            case 'R': positions[white_rook]   |= bit; break;
            case 'Q': positions[white_queen]  |= bit; break;
            case 'K': positions[white_king]   |= bit; break;
        }

        file++;
    }
}

bool Board::isOccupied(int square) {
    if (square < 1 || square > 64) return false;
    return positions[occ] & (1ULL << (square - 1));
}

void Board::takePieceFrom(PieceType pieceType, int square) {
    if (square < 1 || square > 64) return;
    positions[pieceType] &= ~(1ULL << (square - 1));
    _updateOccupancy();
}

void Board::putPieceOn(PieceType pieceType, int square) {
    if (square < 1 || square > 64) return;
    positions[pieceType] |= (1ULL << (square - 1));
    _updateOccupancy();
}

void Board::_updateOccupancy() {
    positions[white_occ] = (
        positions[white_pawn] |
        positions[white_knight] |
        positions[white_bishop] |
        positions[white_rook] |
        positions[white_queen] |
        positions[white_king]
    );

    positions[black_occ] = (
        positions[black_pawn] |
        positions[black_knight] |
        positions[black_bishop] |
        positions[black_rook] |
        positions[black_queen] |
        positions[black_king]
    );

    positions[occ] = positions[white_occ] | positions[black_occ];
}

bool Board::isWhiteTurn() {
    return _packed_info & 1;
}

bool Board::whiteCanCastleKS() {
    return (_packed_info >> 1) & 1;
}

bool Board::whiteCanCastleQS() {
    return (_packed_info >> 2) & 1;
}

bool Board::blackCanCastleKS() {
    return (_packed_info >> 3) & 1;
}

bool Board::blackCanCastleQS() {
    return (_packed_info >> 4) & 1;
}

int Board::getPieceAt(int square) const {
    if (square < 1 || square > 64) return -1;
    uint64_t mask = 1ULL << (square - 1);
    
    // Check each piece type
    for (int i = white_pawn; i <= black_king; i++) {
        if (positions[i] & mask) return i;
    }
    return -1; // Empty square
}

void Board::toogleTurn() {
    _packed_info ^= 0x1;
}

void Board::setTurn(bool isWhite) {
    if (isWhite) _packed_info |= 1;
    else _packed_info &= ~1;
}

void Board::setWhiteCanCastleKS(bool can) {
    if (can) _packed_info |= (1 << 1);
    else _packed_info &= ~(1 << 1);
}

void Board::setWhiteCanCastleQS(bool can) {
    if (can) _packed_info |= (1 << 2);
    else _packed_info &= ~(1 << 2);
}

void Board::setBlackCanCastleKS(bool can) {
    if (can) _packed_info |= (1 << 3);
    else _packed_info &= ~(1 << 3);
}

void Board::setBlackCanCastleQS(bool can) {
    if (can) _packed_info |= (1 << 4);
    else _packed_info &= ~(1 << 4);
}

void Board::copyPositions(uint64_t dest[16]) const {
    std::memcpy(dest, positions, sizeof(uint64_t) * 16);
}

void Board::restorePositions(const uint64_t src[16]) {
    std::memcpy(positions, src, sizeof(uint64_t) * 16);
}

bool Board::makeMove(const Move& move) {
    UndoInfo undo;
    undo.old_en_passant = positions[en_passant];
    undo.old_packed_info = _packed_info;
    undo.move = move;
    undo.captured_piece_type = static_cast<PieceType>(-1);
    undo.captured_piece_bb = 0;
    
    int from_sq = move.from - 1;  // Convert to 0-based
    int to_sq = move.to - 1;
    
    // Find which piece is moving
    int movingPieceInt = getPieceAt(move.from);
    if (movingPieceInt == -1) return false;
    
    PieceType movingPiece = static_cast<PieceType>(movingPieceInt);
    bool isWhite = (movingPiece <= white_king);
    
    // Handle captures (normal capture)
    int capturedInt = getPieceAt(move.to);
    if (capturedInt != -1) {
        undo.captured_piece_type = static_cast<PieceType>(capturedInt);
        undo.captured_piece_bb = positions[capturedInt];
        positions[capturedInt] &= ~(1ULL << to_sq);
    }
    
    // Move the piece
    positions[movingPiece] &= ~(1ULL << from_sq);
    positions[movingPiece] |= (1ULL << to_sq);
    
    // Clear en passant
    positions[en_passant] = 0;
    
    // Handle special moves
    switch (move.type) {
        case EN_PASSANT: {
            // Remove the captured pawn
            int capturedPawnSq = isWhite ? to_sq - 8 : to_sq + 8;
            PieceType capturedPawn = isWhite ? black_pawn : white_pawn;
            undo.captured_piece_type = capturedPawn;
            undo.captured_piece_bb = positions[capturedPawn];
            positions[capturedPawn] &= ~(1ULL << capturedPawnSq);
            break;
        }
        
        case CASTLING: {
            // Move the rook
            int rookFrom, rookTo;
            PieceType rook = isWhite ? white_rook : black_rook;
            
            if (to_sq > from_sq) {
                // Kingside
                rookFrom = isWhite ? 7 : 63;
                rookTo = isWhite ? 5 : 61;
            } else {
                // Queenside
                rookFrom = isWhite ? 0 : 56;
                rookTo = isWhite ? 3 : 59;
            }
            
            positions[rook] &= ~(1ULL << rookFrom);
            positions[rook] |= (1ULL << rookTo);
            break;
        }
        
        case PROMOTION: {
            // Remove pawn
            positions[movingPiece] &= ~(1ULL << to_sq);
            // Add promoted piece
            positions[move.promotionPiece] |= (1ULL << to_sq);
            break;
        }
        
        default:
            break;
    }
    
    // Set en passant square for double pawn push
    if ((movingPiece == white_pawn || movingPiece == black_pawn)) {
        int rankDiff = (to_sq / 8) - (from_sq / 8);
        if (rankDiff == 2 || rankDiff == -2) {
            int epSquare = (from_sq + to_sq) / 2;
            positions[en_passant] = 1ULL << epSquare;
        }
    }
    
    // Update castling rights
    // King moves
    if (movingPiece == white_king) {
        setWhiteCanCastleKS(false);
        setWhiteCanCastleQS(false);
    }
    if (movingPiece == black_king) {
        setBlackCanCastleKS(false);
        setBlackCanCastleQS(false);
    }
    
    // Rook moves or is captured
    if (from_sq == 0 || to_sq == 0) setWhiteCanCastleQS(false);
    if (from_sq == 7 || to_sq == 7) setWhiteCanCastleKS(false);
    if (from_sq == 56 || to_sq == 56) setBlackCanCastleQS(false);
    if (from_sq == 63 || to_sq == 63) setBlackCanCastleKS(false);
    
    // Update half-move clock
    if (movingPiece == white_pawn || movingPiece == black_pawn || capturedInt != -1) {
        half_clock = 0;
    } else {
        half_clock++;
    }
    
    // Toggle turn
    toogleTurn();
    
    // Update occupancy
    _updateOccupancy();
    
    _undo_stack.push_back(undo);
    
    return true;
}

void Board::unmakeMove() {
    if (_undo_stack.empty()) return;
    
    UndoInfo undo = _undo_stack.back();
    _undo_stack.pop_back();
    
    const Move& move = undo.move;
    int from_sq = move.from - 1;
    int to_sq = move.to - 1;
    
    // Find which piece moved (it's now at the 'to' square)
    int movingPieceInt = getPieceAt(move.to);
    
    // Handle promotion - the piece at 'to' is the promoted piece, not the pawn
    if (move.type == PROMOTION) {
        // Remove promoted piece
        positions[move.promotionPiece] &= ~(1ULL << to_sq);
        
        // Restore the pawn
        bool isWhite = (move.promotionPiece <= white_king);
        PieceType pawn = isWhite ? white_pawn : black_pawn;
        positions[pawn] |= (1ULL << from_sq);
    } else if (movingPieceInt != -1) {
        PieceType movingPiece = static_cast<PieceType>(movingPieceInt);
        
        // Move piece back
        positions[movingPiece] &= ~(1ULL << to_sq);
        positions[movingPiece] |= (1ULL << from_sq);
    }
    
    // Restore captured piece
    if (static_cast<int>(undo.captured_piece_type) != -1) {
        if (move.type == EN_PASSANT) {
            // En passant capture - restore pawn to its original square
            bool wasWhiteMoving = !(undo.old_packed_info & 1);  // Turn was toggled
            int capturedPawnSq = wasWhiteMoving ? to_sq - 8 : to_sq + 8;
            positions[undo.captured_piece_type] |= (1ULL << capturedPawnSq);
        } else {
            // Normal capture
            positions[undo.captured_piece_type] |= (1ULL << to_sq);
        }
    }
    
    // Handle castling - move rook back
    if (move.type == CASTLING) {
        bool wasWhiteMoving = !(undo.old_packed_info & 1);
        PieceType rook = wasWhiteMoving ? white_rook : black_rook;
        
        int rookFrom, rookTo;
        if (to_sq > from_sq) {
            // Kingside
            rookFrom = wasWhiteMoving ? 7 : 63;
            rookTo = wasWhiteMoving ? 5 : 61;
        } else {
            // Queenside
            rookFrom = wasWhiteMoving ? 0 : 56;
            rookTo = wasWhiteMoving ? 3 : 59;
        }
        
        // Move rook back
        positions[rook] &= ~(1ULL << rookTo);
        positions[rook] |= (1ULL << rookFrom);
    }
    
    // Restore state
    positions[en_passant] = undo.old_en_passant;
    _packed_info = undo.old_packed_info;
    
    _updateOccupancy();
}
