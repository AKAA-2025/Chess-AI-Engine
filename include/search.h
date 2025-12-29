#pragma once

#include "board.h"
#include "generator.h"

namespace Search {

class Worker {
    Worker(Board board, MoveGenerator movgen);

    float iterative_minimax(int depth, float alpha,
                            float beta, bool maximizing_player);

    float recursive_minimax(int depth, float alpha,
                            float beta, bool maximizing_player);
};

void make_move();
void undo_move();

};