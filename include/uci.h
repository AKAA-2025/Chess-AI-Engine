#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include "board.h"
#include "generator.h"
#include "eval.h"
#include "search.h"

namespace UCI {

/**
 * Engine configuration options
 */
struct EngineOptions {
    int hashSize = 128;        // Hash table size in MB
    int threads = 1;           // Number of search threads
    bool ownBook = false;      // Use own opening book
    int contempt = 0;          // Contempt factor
};

/**
 * Main chess engine class that manages the game state and search
 */
class ChessEngine {
public:
    ChessEngine();
    ~ChessEngine();
    
    /**
     * Initialize engine components
     */
    void init();
    
    /**
     * Start a new game (reset position and state)
     */
    void newGame();
    
    /**
     * Set the current position
     * @param fen The FEN string (or "startpos")
     * @param moves List of moves in algebraic notation to apply
     */
    void setPosition(const std::string& fen, const std::vector<std::string>& moves);
    
    /**
     * Start searching for the best move
     * @param limits Search constraints and time management
     */
    void startSearch(const Search::SearchLimits& limits);
    
    /**
     * Stop the current search
     */
    void stopSearch();
    
    /**
     * Check if engine is currently searching
     */
    bool isSearching() const { return searching; }
    
    /**
     * Check if engine should quit
     */
    bool shouldQuit() const { return quit; }
    
    /**
     * Signal engine to quit
     */
    void setQuit() { quit = true; }
    
    /**
     * Set engine options
     */
    void setOption(const std::string& name, const std::string& value);
    
    /**
     * Get current engine options
     */
    const EngineOptions& getOptions() const { return options; }

private:
    std::unique_ptr<Board> board;
    std::unique_ptr<MoveGenerator::Worker> moveGen;
    std::unique_ptr<Eval::Worker> evaluator;
    std::unique_ptr<Search::Worker> searcher;
    
    EngineOptions options;
    std::atomic<bool> searching;
    std::atomic<bool> quit;
    std::thread searchThread;
    
    /**
     * The actual search function that runs in a separate thread
     */
    void searchThreadFunc(const Search::SearchLimits& limits);
    
    /**
     * Apply a move in algebraic notation to the board
     */
    bool applyMove(const std::string& moveStr);
    
    /**
     * Recreate workers after board change
     */
    void recreateWorkers();
};

/**
 * UCI Protocol handler class
 */
class Protocol {
public:
    Protocol();
    
    /**
     * Main UCI loop - reads commands from stdin and responds
     */
    void run();

private:
    ChessEngine engine;
    
    /**
     * Handle the 'uci' command - identify engine
     */
    void handleUCI();
    
    /**
     * Handle the 'isready' command
     */
    void handleIsReady();
    
    /**
     * Handle the 'ucinewgame' command
     */
    void handleNewGame();
    
    /**
     * Handle the 'position' command
     * @param input Remaining input stream after 'position'
     */
    void handlePosition(std::istringstream& input);
    
    /**
     * Handle the 'go' command with search parameters
     * @param input Remaining input stream after 'go'
     */
    void handleGo(std::istringstream& input);
    
    /**
     * Handle the 'stop' command
     */
    void handleStop();
    
    /**
     * Handle the 'setoption' command
     * @param input Remaining input stream after 'setoption'
     */
    void handleSetOption(std::istringstream& input);
    
    /**
     * Handle the 'quit' command
     */
    void handleQuit();
    
    /**
     * Handle the 'd' (display) debug command
     */
    void handleDisplay();
    
    /**
     * Handle the 'perft' command for testing
     */
    void handlePerft(std::istringstream& input);
    
    /**
     * Parse FEN and moves from position command
     */
    void parsePosition(std::istringstream& input, std::string& fen, 
                      std::vector<std::string>& moves);
    
    /**
     * Parse search limits from go command
     */
    Search::SearchLimits parseGoLimits(std::istringstream& input);
};

/**
 * Utility functions for UCI protocol
 */
namespace Utils {
    /**
     * Convert a Move to UCI format (e.g., "e2e4", "e7e8q")
     */
    std::string moveToUCI(const Move& move);
    
    /**
     * Parse a UCI move string to a Move object
     */
    Move parseUCIMove(const std::string& uciMove, Board* board);
    
    /**
     * Send the best move found
     */
    void sendBestMove(const Move& bestMove, const Move& ponderMove = Move());
}

} // namespace UCI
