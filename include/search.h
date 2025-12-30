#pragma once

#include "board.h"
#include "generator.h"
#include "eval.h"

namespace Search {

class Worker {
public:
    Worker(MoveGenerator::Worker gen, Eval::Worker evaluator);
    void recursiveAlphaBeta();
    void iterativeAlphaBeta();
};

}