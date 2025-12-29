#pragma once
#include <cstdint>
#include "moves.h"

#define RANK_1 0x00000000000000FF
#define RANK_2 0x000000000000FF00
#define RANK_3 0x0000000000FF0000
#define RANK_4 0x00000000FF000000
#define RANK_5 0x000000FF00000000
#define RANK_6 0x0000FF0000000000
#define RANK_7 0x00FF000000000000
#define RANK_8 0xFF00000000000000

#define FILE_A 0x0101010101010101
#define FILE_B 0x0202020202020202
#define FILE_C 0x0404040404040404
#define FILE_D 0x0808080808080808
#define FILE_E 0x1010101010101010
#define FILE_F 0x2020202020202020
#define FILE_G 0x4040404040404040
#define FILE_H 0x8080808080808080

typedef enum {
    white_pawn,
    white_rook,
    white_knight,
    white_bishop,
    white_queen,
    white_king,

    black_pawn,
    black_rook,
    black_knight,
    black_bishop,
    black_queen,
    black_king,

    white_occ,
    black_occ,
    occ,

    en_passant
} PieceType;

struct UndoInfo {
    uint64_t captured_piece_bb;
    PieceType captured_piece_type;
    uint64_t old_en_passant;
    uint8_t old_packed_info;
    Move move;
};

class Board {
public:
    Board();
    
    uint64_t positions[16] = {0UL};

    /**
     * @param piece The piece to check for (e.g. black_rooks)
     * @param square The square to check (1-64)
     */
    bool isOccupied(int square);
    /**
     * @param piece The piece (e.g. black_rooks)
     * @param square The square (1-64)
     */
    void takePieceFrom(PieceType pieceType, int square);
    /**
     * @param piece The piece (e.g. black_rooks)
     * @param square The square (1-64)
     */
    void putPieceOn(PieceType pieceType, int square);

    bool isWhiteTurn();
    bool whiteCanCastleKS();
    bool whiteCanCastleQS();
    bool blackCanCastleKS();
    bool blackCanCastleQS();

     /**
     * Make a move on the board (updates bitboards and game state)
     * @param move The move to make
     * @return true if move was successfully made
     */
    bool makeMove(const Move& move);
    
    /**
     * Take back the last move
     */
    void unmakeMove();
    
    /**
     * Get the piece type at a given square
     * @param square The square (1-64)
     * @return PieceType or -1 if empty
     */
    int getPieceAt(int square) const;
    
    /**
     * Update packed info (call this after modifying _packed_info bits)
     */
    void toogleTurn();
    void setWhiteCanCastleKS(bool can);
    void setWhiteCanCastleQS(bool can);
    void setBlackCanCastleKS(bool can);
    void setBlackCanCastleQS(bool can);
    
    /**
     * Get a copy of positions (useful for saving state)
     */
    void copyPositions(uint64_t dest[16]) const;
    void restorePositions(const uint64_t src[16]);
    
    /**
     * Get packed info (for saving/restoring state)
     */
    uint8_t getPackedInfo() const { return _packed_info; }
    void setPackedInfo(uint8_t info) { _packed_info = info; }

private:
    /**
     * From LSB to MSB
     * 
     * Bit 1 : Turn to move (1 = white, 0 = black)
     * 
     * For bit 1-4, 0 means false and 1 means true
     * Bit 2 : White can castle king side
     * Bit 3 : White can castle queen side
     * Bit 4 : Black can castle king side
     * Bit 5 : Black can castle queen side
     */
    uint8_t _packed_info;
    std::vector<UndoInfo> _undo_stack;

    void _updateOccupancy();
};