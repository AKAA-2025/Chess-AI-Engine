#include "board.h"
#include "generator.h"
#include <iostream>

int main() {
    // Initialize board
    Board board;
    
    // Create move generator
    MoveGenerator::Worker generator(&board);
    
    // Generate all pseudo-legal moves
    std::vector<Move> pseudoMoves = generator.generateAllMoves();
    
    std::cout << "Pseudo-legal moves: " << pseudoMoves.size() << std::endl;
    for (const auto& move : pseudoMoves) {
        std::cout << move.notation << " (from: " << move.from 
                  << ", to: " << move.to << ")" << std::endl;
    }
    
    // Filter to only legal moves
    std::vector<Move> legalMoves = generator.filterLegalMoves(pseudoMoves);
    std::cout << "\nLegal moves: " << legalMoves.size() << std::endl;
    
    // Check if in check
    if (generator.isInCheck()) {
        std::cout << "King is in check!" << std::endl;
    }
    
    // Generate only captures
    std::vector<Move> captures = generator.generateCaptures();
    std::cout << "\nCapture moves: " << captures.size() << std::endl;
    
    // Check if a specific move is legal
    Move testMove;
    testMove.from = 12; // e2 (if using 1-64 indexing)
    testMove.to = 28;   // e4
    testMove.notation = "e2e4";
    
    if (generator.isPseudoLegal(testMove)) {
        std::cout << "\nMove e2-e4 is pseudo-legal" << std::endl;
        if (generator.isLegal(testMove)) {
            std::cout << "Move e2-e4 is legal" << std::endl;
        }
    }
    
    // Check if a square is attacked
    bool e4Attacked = generator.isSquareAttacked(27, false); // Check if black attacks e4
    std::cout << "\nE4 attacked by black: " << (e4Attacked ? "yes" : "no") << std::endl;
    
    return 0;
}