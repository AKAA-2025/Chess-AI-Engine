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
 * Search limits and parameters
 */
struct SearchLimits {
    int depth = -1;            // Maximum depth (-1 = unlimited)
    int movetime = -1;         // Time per move in ms (-1 = unlimited)
    long long nodes = -1;      // Maximum nodes (-1 = unlimited)
    int wtime = -1;            // White time remaining in ms
    int btime = -1;            // Black time remaining in ms
    int winc = 0;              // White increment per move in ms
    int binc = 0;              // Black increment per move in ms
    int movestogo = -1;        // Moves until next time control
    bool infinite = false;     // Search until stop command
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
    void startSearch(const SearchLimits& limits);
    
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
    void searchThreadFunc(const SearchLimits& limits);
    
    /**
     * Calculate time allocation for this move based on time controls
     */
    int calculateMoveTime(const SearchLimits& limits);
    
    /**
     * Apply a move in algebraic notation to the board
     */
    bool applyMove(const std::string& moveStr);
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
     * Parse FEN and moves from position command
     */
    void parsePosition(std::istringstream& input, std::string& fen, 
                      std::vector<std::string>& moves);
    
    /**
     * Parse search limits from go command
     */
    SearchLimits parseGoLimits(std::istringstream& input);
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
     * Send info output during search
     */
    void sendInfo(int depth, int score, long long nodes, int time, 
                  const std::vector<Move>& pv);
    
    /**
     * Send the best move found
     */
    void sendBestMove(const Move& bestMove, const Move& ponderMove = Move());
}

} // namespace UCI