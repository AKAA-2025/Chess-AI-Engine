#include "uci.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>

namespace UCI {

// ============================================================================
// ChessEngine Implementation
// ============================================================================

ChessEngine::ChessEngine() 
    : searching(false), quit(false) {
}

ChessEngine::~ChessEngine() {
    stopSearch();
}

void ChessEngine::init() {
    // Initialize attack tables first
    MoveGenerator::AttackTables::initialize();
    
    // Create initial position
    board = std::make_unique<Board>();
    moveGen = std::make_unique<MoveGenerator::Worker>(board.get());
    evaluator = std::make_unique<Eval::Worker>(board.get());
    searcher = std::make_unique<Search::Worker>(*moveGen, *evaluator);
}

void ChessEngine::newGame() {
    stopSearch();
    
    // Reset to starting position
    board = std::make_unique<Board>();
    moveGen = std::make_unique<MoveGenerator::Worker>(board.get());
    evaluator = std::make_unique<Eval::Worker>(board.get());
    searcher = std::make_unique<Search::Worker>(*moveGen, *evaluator);
}

void ChessEngine::setPosition(const std::string& fen, const std::vector<std::string>& moves) {
    // Set position from FEN
    if (fen == "startpos") {
        board = std::make_unique<Board>();
    } else {
        board = std::make_unique<Board>(fen.c_str());
    }
    
    // Recreate workers with new board
    moveGen = std::make_unique<MoveGenerator::Worker>(board.get());
    evaluator = std::make_unique<Eval::Worker>(board.get());
    searcher = std::make_unique<Search::Worker>(*moveGen, *evaluator);
    
    // Apply moves
    for (const auto& moveStr : moves) {
        if (!applyMove(moveStr)) {
            std::cerr << "info string Invalid move: " << moveStr << std::endl;
            break;
        }
    }
}

bool ChessEngine::applyMove(const std::string& moveStr) {
    // Parse UCI move (e.g., "e2e4", "e7e8q")
    if (moveStr.length() < 4) return false;
    
    int fromSquare = (moveStr[0] - 'a') + (moveStr[1] - '1') * 8;
    int toSquare = (moveStr[2] - 'a') + (moveStr[3] - '1') * 8;
    
    // Get all legal moves and find matching one
    std::vector<Move> moves = moveGen->generateAllMoves();
    std::vector<Move> legalMoves = moveGen->filterLegalMoves(moves);
    
    for (const auto& move : legalMoves) {
        // Compare from and to squares
        // Note: You'll need to add methods to Move class to get from/to squares
        // or parse the move string format your Move class uses
        
        // For now, try to make the move and see if it works
        Move candidate = Utils::parseUCIMove(moveStr, board.get());
        if (board->makeMove(candidate)) {
            return true;
        }
    }
    
    return false;
}

void ChessEngine::startSearch(const SearchLimits& limits) {
    if (searching) stopSearch();
    
    searching = true;
    
    // Launch search in separate thread
    searchThread = std::thread([this, limits]() {
        searchThreadFunc(limits);
    });
    
    searchThread.detach();
}

void ChessEngine::searchThreadFunc(const SearchLimits& limits) {
    auto startTime = std::chrono::steady_clock::now();
    
    // Calculate time allocation if needed
    int allocatedTime = calculateMoveTime(limits);
    
    // TODO: Replace this with your actual search implementation
    // For now, just do a simple search
    
    std::vector<Move> allMoves = moveGen->generateAllMoves();
    std::vector<Move> legalMoves = moveGen->filterLegalMoves(allMoves);
    
    if (legalMoves.empty()) {
        searching = false;
        return;
    }
    
    // Simple evaluation of each move
    Move bestMove = legalMoves[0];
    int bestScore = -999999;
    
    for (const auto& move : legalMoves) {
        // Make move
        board->makeMove(move);
        
        // Evaluate
        int score = -evaluator->evaluate();
        
        // Unmake move
        board->unmakeMove();
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        
        // Check time limit
        if (allocatedTime > 0) {
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            if (elapsedMs >= allocatedTime) {
                break;
            }
        }
    }
    
    // Send best move
    Utils::sendBestMove(bestMove);
    
    searching = false;
}

int ChessEngine::calculateMoveTime(const SearchLimits& limits) {
    // If movetime is specified, use it directly
    if (limits.movetime > 0) {
        return limits.movetime;
    }
    
    // If infinite search, return -1
    if (limits.infinite) {
        return -1;
    }
    
    // Calculate from time controls
    int timeRemaining = board->isWhiteTurn() ? limits.wtime : limits.btime;
    int increment = board->isWhiteTurn() ? limits.winc : limits.binc;
    
    if (timeRemaining <= 0) {
        return -1;
    }
    
    // Simple time management: use 1/30 of remaining time + increment
    int movesToGo = (limits.movestogo > 0) ? limits.movestogo : 30;
    int allocatedTime = (timeRemaining / movesToGo) + increment * 0.8;
    
    // Don't use more than 1/5 of remaining time
    allocatedTime = std::min(allocatedTime, timeRemaining / 5);
    
    return allocatedTime;
}

void ChessEngine::stopSearch() {
    if (searching) {
        searching = false;
        
        // Wait for search thread to finish
        if (searchThread.joinable()) {
            searchThread.join();
        }
    }
}

void ChessEngine::setOption(const std::string& name, const std::string& value) {
    if (name == "Hash") {
        try {
            options.hashSize = std::stoi(value);
            // TODO: Resize hash table
        } catch (...) {
            std::cerr << "info string Invalid Hash value: " << value << std::endl;
        }
    } else if (name == "Threads") {
        try {
            options.threads = std::stoi(value);
            // TODO: Update thread count
        } catch (...) {
            std::cerr << "info string Invalid Threads value: " << value << std::endl;
        }
    } else if (name == "OwnBook") {
        options.ownBook = (value == "true");
    } else if (name == "Contempt") {
        try {
            options.contempt = std::stoi(value);
        } catch (...) {
            std::cerr << "info string Invalid Contempt value: " << value << std::endl;
        }
    }
}

// ============================================================================
// Protocol Implementation
// ============================================================================

Protocol::Protocol() {
}

void Protocol::run() {
    engine.init();
    
    std::string line;
    while (std::getline(std::cin, line) && !engine.shouldQuit()) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "uci") {
            handleUCI();
        } else if (command == "isready") {
            handleIsReady();
        } else if (command == "ucinewgame") {
            handleNewGame();
        } else if (command == "position") {
            handlePosition(iss);
        } else if (command == "go") {
            handleGo(iss);
        } else if (command == "stop") {
            handleStop();
        } else if (command == "setoption") {
            handleSetOption(iss);
        } else if (command == "quit") {
            handleQuit();
        }
    }
}

void Protocol::handleUCI() {
    std::cout << "id name ChessEngine 1.0" << std::endl;
    std::cout << "id author YourName" << std::endl;
    
    // Send available options
    std::cout << "option name Hash type spin default 128 min 1 max 16384" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 256" << std::endl;
    std::cout << "option name OwnBook type check default false" << std::endl;
    std::cout << "option name Contempt type spin default 0 min -100 max 100" << std::endl;
    
    std::cout << "uciok" << std::endl;
}

void Protocol::handleIsReady() {
    std::cout << "readyok" << std::endl;
}

void Protocol::handleNewGame() {
    engine.newGame();
}

void Protocol::handlePosition(std::istringstream& input) {
    std::string fen;
    std::vector<std::string> moves;
    
    parsePosition(input, fen, moves);
    engine.setPosition(fen, moves);
}

void Protocol::parsePosition(std::istringstream& input, std::string& fen, 
                             std::vector<std::string>& moves) {
    std::string token;
    input >> token;
    
    if (token == "startpos") {
        fen = "startpos";
        input >> token; // Try to read "moves"
    } else if (token == "fen") {
        // Read FEN string until "moves" or end
        std::ostringstream fenBuilder;
        while (input >> token && token != "moves") {
            if (fenBuilder.tellp() > 0) fenBuilder << " ";
            fenBuilder << token;
        }
        fen = fenBuilder.str();
    }
    
    // Read moves if present
    if (token == "moves" || input >> token) {
        if (token == "moves") {
            input >> token;
        }
        
        do {
            moves.push_back(token);
        } while (input >> token);
    }
}

void Protocol::handleGo(std::istringstream& input) {
    SearchLimits limits = parseGoLimits(input);
    engine.startSearch(limits);
}

SearchLimits Protocol::parseGoLimits(std::istringstream& input) {
    SearchLimits limits;
    std::string token;
    
    while (input >> token) {
        if (token == "depth") {
            input >> limits.depth;
        } else if (token == "movetime") {
            input >> limits.movetime;
        } else if (token == "nodes") {
            input >> limits.nodes;
        } else if (token == "wtime") {
            input >> limits.wtime;
        } else if (token == "btime") {
            input >> limits.btime;
        } else if (token == "winc") {
            input >> limits.winc;
        } else if (token == "binc") {
            input >> limits.binc;
        } else if (token == "movestogo") {
            input >> limits.movestogo;
        } else if (token == "infinite") {
            limits.infinite = true;
        }
    }
    
    return limits;
}

void Protocol::handleStop() {
    engine.stopSearch();
}

void Protocol::handleSetOption(std::istringstream& input) {
    std::string token;
    input >> token; // Should be "name"
    
    if (token != "name") return;
    
    // Read option name (may be multiple words)
    std::ostringstream nameBuilder;
    while (input >> token && token != "value") {
        if (nameBuilder.tellp() > 0) nameBuilder << " ";
        nameBuilder << token;
    }
    std::string optionName = nameBuilder.str();
    
    // Read option value (may be multiple words)
    std::ostringstream valueBuilder;
    while (input >> token) {
        if (valueBuilder.tellp() > 0) valueBuilder << " ";
        valueBuilder << token;
    }
    std::string optionValue = valueBuilder.str();
    
    engine.setOption(optionName, optionValue);
}

void Protocol::handleQuit() {
    engine.stopSearch();
    engine.setQuit();
}

// ============================================================================
// Utils Implementation
// ============================================================================

namespace Utils {

std::string moveToUCI(const Move& move) {
    // TODO: Implement based on your Move class structure
    // This is a placeholder - you'll need to adapt it to your Move format
    
    // Example if Move stores from/to squares:
    // int from = move.getFrom();
    // int to = move.getTo();
    // char fromFile = 'a' + (from % 8);
    // char fromRank = '1' + (from / 8);
    // char toFile = 'a' + (to % 8);
    // char toRank = '1' + (to / 8);
    // 
    // std::string uci;
    // uci += fromFile;
    // uci += fromRank;
    // uci += toFile;
    // uci += toRank;
    // 
    // // Add promotion piece if applicable
    // if (move.isPromotion()) {
    //     uci += move.getPromotionPiece();
    // }
    
    return "e2e4"; // Placeholder
}

Move parseUCIMove(const std::string& uciMove, Board* board) {
    // TODO: Implement based on your Move class structure
    // Parse UCI format like "e2e4" or "e7e8q"
    
    if (uciMove.length() < 4) {
        return Move(); // Invalid
    }
    
    int fromFile = uciMove[0] - 'a';
    int fromRank = uciMove[1] - '1';
    int toFile = uciMove[2] - 'a';
    int toRank = uciMove[3] - '1';
    
    int fromSquare = fromRank * 8 + fromFile;
    int toSquare = toRank * 8 + toFile;
    
    // TODO: Create Move object with these squares
    // Handle promotion if uciMove.length() == 5
    
    return Move(); // Placeholder
}

void sendInfo(int depth, int score, long long nodes, int time, 
              const std::vector<Move>& pv) {
    std::cout << "info";
    std::cout << " depth " << depth;
    std::cout << " score cp " << score;
    std::cout << " nodes " << nodes;
    std::cout << " time " << time;
    std::cout << " nps " << (time > 0 ? (nodes * 1000 / time) : 0);
    
    if (!pv.empty()) {
        std::cout << " pv";
        for (const auto& move : pv) {
            std::cout << " " << moveToUCI(move);
        }
    }
    
    std::cout << std::endl;
}

void sendBestMove(const Move& bestMove, const Move& ponderMove) {
    std::cout << "bestmove " << moveToUCI(bestMove);
    
    // Optionally send ponder move
    // if (ponderMove is valid) {
    //     std::cout << " ponder " << moveToUCI(ponderMove);
    // }
    
    std::cout << std::endl;
}

} // namespace Utils

} // namespace UCI