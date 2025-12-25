#pragma once
#include <cstdint>

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

class Board {
public:
    Board();
    
    uint64_t white_pawns;
    uint64_t white_rooks;
    uint64_t white_knights;
    uint64_t white_bishops;
    uint64_t white_queen;
    uint64_t white_king;

    uint64_t black_pawns;
    uint64_t black_rooks;
    uint64_t black_knights;
    uint64_t black_bishops;
    uint64_t black_queen;
    uint64_t black_king;

    uint64_t white_occ;
    uint64_t black_occ;
    uint64_t occ;

    /**
     * @param piece The piece to check for (e.g. black_rooks)
     * @param square The square to check (1-64)
     */
    bool isOccupied(uint64_t piece, int square);
    /**
     * @param The square to check (1-64)
     */
    void takePieceFrom(uint64_t piece, int square);
    /**
     * @param The square to check (1-64)
     */
    void putPieceOn(uint64_t piece, int square);

private:
    void _updateOccupancy();
};