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
    recreateWorkers();
}

void ChessEngine::recreateWorkers() {
    moveGen = std::make_unique<MoveGenerator::Worker>(board.get());
    evaluator = std::make_unique<Eval::Worker>(board.get());
    searcher = std::make_unique<Search::Worker>(board.get(), moveGen.get(), evaluator.get());
}

void ChessEngine::newGame() {
    stopSearch();
    
    // Reset to starting position
    board = std::make_unique<Board>();
    recreateWorkers();
}

void ChessEngine::setPosition(const std::string& fen, const std::vector<std::string>& moves) {
    // Set position from FEN
    if (fen == "startpos") {
        board = std::make_unique<Board>();
    } else {
        board = std::make_unique<Board>(fen.c_str());
    }
    
    // Recreate workers with new board
    recreateWorkers();
    
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
    
    Move move = Utils::parseUCIMove(moveStr, board.get());
    
    if (move.from == 0 && move.to == 0) {
        return false;
    }
    
    // Verify it's a legal move
    std::vector<Move> pseudoMoves = moveGen->generateAllMoves();
    std::vector<Move> legalMoves = moveGen->filterLegalMoves(pseudoMoves);
    
    for (const auto& legal : legalMoves) {
        if (legal.from == move.from && legal.to == move.to) {
            // Handle promotion
            if (move.type == PROMOTION) {
                // Find the matching promotion piece
                for (const auto& promo : legalMoves) {
                    if (promo.from == move.from && promo.to == move.to && 
                        promo.promotionPiece == move.promotionPiece) {
                        return board->makeMove(promo);
                    }
                }
            }
            return board->makeMove(legal);
        }
    }
    
    return false;
}

void ChessEngine::startSearch(const Search::SearchLimits& limits) {
    if (searching) stopSearch();
    
    searching = true;
    
    // Launch search in separate thread
    searchThread = std::thread([this, limits]() {
        searchThreadFunc(limits);
    });
    
    // Wait for search to complete
    searchThread.join();
}

void ChessEngine::searchThreadFunc(const Search::SearchLimits& limits) {
    Search::SearchResult result = searcher->search(limits);
    
    // Send best move
    if (result.bestMove.from != 0 || result.bestMove.to != 0) {
        Utils::sendBestMove(result.bestMove);
    } else {
        // No legal moves found - might be checkmate or stalemate
        std::vector<Move> pseudoMoves = moveGen->generateAllMoves();
        std::vector<Move> legalMoves = moveGen->filterLegalMoves(pseudoMoves);
        
        if (!legalMoves.empty()) {
            Utils::sendBestMove(legalMoves[0]);
        } else {
            std::cout << "bestmove (none)" << std::endl;
        }
    }
    
    searching = false;
}

void ChessEngine::stopSearch() {
    if (searching && searcher) {
        searcher->stop();
    }
    
    searching = false;
    
    // Wait for search thread to finish
    if (searchThread.joinable()) {
        searchThread.join();
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
        } else if (command == "d") {
            handleDisplay();
        } else if (command == "perft") {
            handlePerft(iss);
        }
    }
}

void Protocol::handleUCI() {
    std::cout << "id name ChessAI 1.0" << std::endl;
    std::cout << "id author Ranadi" << std::endl;
    
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
    if (token == "moves") {
        while (input >> token) {
            moves.push_back(token);
        }
    }
}

void Protocol::handleGo(std::istringstream& input) {
    Search::SearchLimits limits = parseGoLimits(input);
    engine.startSearch(limits);
}

Search::SearchLimits Protocol::parseGoLimits(std::istringstream& input) {
    Search::SearchLimits limits;
    std::string token;
    
    while (input >> token) {
        if (token == "depth") {
            input >> limits.maxDepth;
        } else if (token == "movetime") {
            input >> limits.moveTime;
        } else if (token == "nodes") {
            input >> limits.maxNodes;
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

void Protocol::handleDisplay() {
    // Debug command to display current board (not part of UCI standard)
    std::cout << "info string Board display not implemented" << std::endl;
}

void Protocol::handlePerft(std::istringstream& input) {
    // Perft command for testing move generation
    int depth = 1;
    input >> depth;
    std::cout << "info string Perft not implemented" << std::endl;
}

// ============================================================================
// Utils Implementation
// ============================================================================

namespace Utils {

std::string moveToUCI(const Move& move) {
    std::string uci;
    
    int fromFile = (move.from - 1) % 8;
    int fromRank = (move.from - 1) / 8;
    int toFile = (move.to - 1) % 8;
    int toRank = (move.to - 1) / 8;
    
    uci += static_cast<char>('a' + fromFile);
    uci += static_cast<char>('1' + fromRank);
    uci += static_cast<char>('a' + toFile);
    uci += static_cast<char>('1' + toRank);
    
    // Add promotion piece if applicable
    if (move.type == PROMOTION) {
        switch (move.promotionPiece) {
            case white_queen: case black_queen: uci += "q"; break;
            case white_rook: case black_rook: uci += "r"; break;
            case white_bishop: case black_bishop: uci += "b"; break;
            case white_knight: case black_knight: uci += "n"; break;
            default: break;
        }
    }
    
    return uci;
}

Move parseUCIMove(const std::string& uciMove, Board* board) {
    if (uciMove.length() < 4) {
        return Move();
    }
    
    int fromFile = uciMove[0] - 'a';
    int fromRank = uciMove[1] - '1';
    int toFile = uciMove[2] - 'a';
    int toRank = uciMove[3] - '1';
    
    // Validate coordinates
    if (fromFile < 0 || fromFile > 7 || fromRank < 0 || fromRank > 7 ||
        toFile < 0 || toFile > 7 || toRank < 0 || toRank > 7) {
        return Move();
    }
    
    int fromSquare = fromRank * 8 + fromFile + 1;  // 1-based
    int toSquare = toRank * 8 + toFile + 1;        // 1-based
    
    Move move(fromSquare, toSquare, uciMove);
    
    // Detect move type
    int piece = board->getPieceAt(fromSquare);
    int captured = board->getPieceAt(toSquare);
    
    // Check for capture
    if (captured != -1) {
        move.type = CAPTURE;
    }
    
    // Check for castling
    if (piece == white_king || piece == black_king) {
        int fileDiff = toFile - fromFile;
        if (std::abs(fileDiff) == 2) {
            move.type = CASTLING;
        }
    }
    
    // Check for pawn special moves
    if (piece == white_pawn || piece == black_pawn) {
        // En passant
        if (fromFile != toFile && captured == -1) {
            move.type = EN_PASSANT;
        }
        
        // Promotion
        if (toRank == 7 || toRank == 0) {
            move.type = PROMOTION;
            
            // Default to queen promotion
            PieceType promoBase = (piece == white_pawn) ? white_queen : black_queen;
            
            if (uciMove.length() >= 5) {
                char promo = uciMove[4];
                bool isWhite = (piece == white_pawn);
                
                switch (promo) {
                    case 'q': move.promotionPiece = isWhite ? white_queen : black_queen; break;
                    case 'r': move.promotionPiece = isWhite ? white_rook : black_rook; break;
                    case 'b': move.promotionPiece = isWhite ? white_bishop : black_bishop; break;
                    case 'n': move.promotionPiece = isWhite ? white_knight : black_knight; break;
                    default: move.promotionPiece = promoBase; break;
                }
            } else {
                move.promotionPiece = promoBase;
            }
        }
    }
    
    return move;
}

void sendBestMove(const Move& bestMove, const Move& ponderMove) {
    std::cout << "bestmove " << moveToUCI(bestMove);
    
    // Optionally send ponder move
    if (ponderMove.from != 0 && ponderMove.to != 0) {
        std::cout << " ponder " << moveToUCI(ponderMove);
    }
    
    std::cout << std::endl;
}

} // namespace Utils

} // namespace UCI
