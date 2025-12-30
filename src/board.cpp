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

    _packed_info = 0x1F;

    _updateOccupancy();
}

Board::Board(const char *fen) {
    std::memset(positions, 0, sizeof(positions));

    char buf[256];
    std::strncpy(buf, fen, sizeof(buf));

    char *token = strtok(buf, " ");
    if (!token) return;
    _fenImportBoard(token);

    token = strtok(NULL, " ");
    if (!token) return;
    setTurn(token[0] == 'w');
    
    token = strtok(NULL, " ");
    if (!token) return;
    setPackedInfo(0x00);
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
        positions[en_passant] |= 1ULL << ((token[0] - 'a') + (token[1] - '1') * 8);
    }

    token = strtok(NULL, " ");
    if (!token) return;
    half_clock = std::atoi(token);

    // Full move counter is ignored since it doesnt help the engine

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

    return positions[occ] & (1ULL << square-1);
}

void Board::takePieceFrom(PieceType pieceType, int square) {
    if (square < 1 || square > 64) return;

    positions[pieceType] &= ~(1ULL << square-1);
    _updateOccupancy();
}

void Board::putPieceOn(PieceType pieceType, int square) {
    if (square < 1 || square > 64) return;

    positions[pieceType] |= (1ULL << square-1);
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

    positions[occ] = positions[white_occ] + positions[black_occ];
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
    if (isWhite) _packed_info |= (1);
    else _packed_info &= ~(1);
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
    // Implementation needed - this is complex and depends on your move format
    // This should:
    // 1. Remove piece from 'from' square
    // 2. Place piece on 'to' square
    // 3. Handle captures
    // 4. Update castling rights
    // 5. Update en passant square
    // 6. Toggle turn
    // 7. Call _updateOccupancy()

    UndoInfo undo;
    undo.old_en_passant = positions[en_passant];
    undo.old_packed_info = _packed_info;
    undo.move = move;
    
    int from_sq = move.from;
    int to_sq = move.to;
    
    PieceType moving_piece = static_cast<PieceType>(getPieceAt(move.from));
    
    // Handle captures
    int captured = getPieceAt(move.to);
    if (captured != -1) {
        undo.captured_piece_type = static_cast<PieceType>(captured);
        undo.captured_piece_bb = positions[captured];
        takePieceFrom(static_cast<PieceType>(captured), to_sq);
    }
    
    // Move the piece
    takePieceFrom(moving_piece, from_sq);
    putPieceOn(moving_piece, to_sq);
    
    // Handle special moves (castling, en passant, promotion)
    // ... implement these ...
    
    // Update en passant
    positions[en_passant] = 0;
    // If pawn double push, set en passant square
    
    // Update castling rights
    // ... implement these ...
    
    // Toggle turn
    toogleTurn();
    
    // Update occupancy
    _updateOccupancy();
    
    _undo_stack.push_back(undo);
    
    return true;
}