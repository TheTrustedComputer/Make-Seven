/*
    Copyright (C) 2023- TheTrustedComputer
    
    Monte Carlo tree search for Make 7.
    
    A popular game-playing algorithm not unlike minimax but uses random simulated games to determine the best move.
    Simple as 1-2-3-4: select, expand, simulate, and backpropagate. Repeat until either timeout or reaching memory limit.
    
    This implementation uses a points-based reward system than a naive win-rate statistic, encouraging a stronger level of play.
    Wins are worth +1 point, losses -1 point, and draws 0 points. Immediate wins or losses are worth one plus the game grid's area minus the depth.
    It is still not immune to shallow traps; a good short-term strategy becomes a bad long-term strategy; this is a weakness of random sampling.
    
    Despite the weaknesses of Monte Carlo tree search, it is still a very useful for three reasons:
    - Best suited for games with large branching factors where traditional minimax becomes impractical to use.
    - No evaluation function is necessary; the algorithm learns from the results of the simulations.
    - Highly parallelizable! It is difficult to parallelize minimax due to its inherently serial nature.
    
    Monte Carlo tree search can be parallelized in three different ways:
    1. Root parallelization - Runs multiple instances of the algorithm with a global root to combine the results, easy to write but more memory intensive.
    2. Leaf parallelization - Threads simulate games from the same leaf node and backpropagate the results simultaneously, highly unbalanced and not scalable.
    3. Tree parallelization - Each threads works on a different branch of the tree independently, requires synchronization to protect it from race conditions.
*/

#ifndef MCTS_H
#define MCTS_H

#include <time.h>
#include <math.h>
#include <signal.h>
#include <limits.h>

#include "make7.h"

// sqrt(2) is a good balance between exploration and exploitation
// smalller = more exploitation; larger = more exploration
#define MCTS_UCT_C 1.41421356237309504880168872420969807856967187537694807317667973799073247846210703885l
#define MCTS_INVALID -100.0l // Valid range is [-25, 25]

// Flag to run the search via Ctrl-C
static atomic_bool runMCTS = true;

// The data structure housing a Monte Carlo tree search node
// The game is not stored to reduce memory footprint when running for long periods
// Rather, it is updated as the algorithm progresses
typedef struct MCTSNode
{
    signed long long points;
    unsigned long long visits;
    struct MCTSNode *ancestor, *descendant;
    uint8_t move, count;
}
MCTSNode;

// Data structure to return the best move and its mean points
typedef struct
{
    long double meanPts;
    uint8_t bestMove;
}
MCTSResult;

// MCTSNode root parallelization worker thread
typedef struct
{
    Make7 copyM7;
    MCTSNode localRoot, *globalRoot;
    Make7 *originM7;
    mtx_t *gRootLock;
    atomic_ullong *iters;
    int id;
    uint8_t totalGMoves;
}
MCTSRootThread;

/*
// Leaf parallelization worker thread
typedef struct
{
    Make7 simulMS;
    MCTSNode *backprop;
    long long *simScore, *simResult;
    int id;
    uint8_t turn;
}
MCTSLeafThread;

// Work queue for leaf parallelization
typedef struct
{
    MCTSLeafThread *items;
    size_t size, capacity, front, back;
    mtx_t *lock;
    cnd_t *notEmpty, *notFull;
}
MTCSWorkQueue;
*/

// Memory management
void MCTSNode_initialize(MCTSNode*, MCTSNode*, const uint8_t);                                                  // Initialize a Monte Carlo tree search node
void MCTSNode_destroy(MCTSNode*);                                                                               // Recursively release memory from it and its descendants

// Functions for debugging
void MCTSNode_print(const MCTSNode*);                                                                           // Print this node's current information
void MCTSNode_printAll(const MCTSNode*, const int);                                                             // Print all of the nodes of the Monte Carlo tree
void MCTSNode_avgPoints(const MCTSNode*);                                                                       // Print the average point per visits of descendant nodes

// Monte Carlo tree search
long double MCTS_uct(const MCTSNode*);                                                                          // Compute the upper confidence bound for trees
MCTSNode *MCTS_select(MCTSNode*, Make7*);                                                                       // Select the best node to expand
bool MCTS_expand(MCTSNode*, const Make7*);                                                                      // Expand the selected node for every move
signed long long MCTS_simulate(Make7*, const uint8_t);                                                          // Simulate a random game from the current state
void MCTS_backpropagate(MCTSNode*, signed long long);                                                           // Backpropagate the result of the simulation
MCTSResult MCTS_best(const MCTSNode*);                                                                          // Return the best move from the root node
uint8_t MCTS_search(const Make7*, void*, const bool);                                                     // Entry point for the Monte Carlo tree search algorithm
void MCTS_progress(const MCTSResult*, void*, const unsigned long long, const unsigned long long);         // Write the Monte Carlo tree search progress to stdout
void MCTS_pointStats(void*, const long double*, const long double*, const long double*);                  // Display the cumulative point statistics for each move
static inline void MCTS_stop(int);                                                                              // Toggle the run flag to stop after receiving SIGINT

// Multi-threading
void MCTSNode_update(MCTSNode*, MCTSNode*, const uint8_t);                                                      // Update the global root node's statistics
int MCTS_rootWorker(void*);                                                                                     // Main function for the Monte Carlo tree search worker
uint8_t MCTS_rootParallel(Make7*, void*, const bool);                                                     // Run the algorithm in parallel at the root node

#endif /* MCTS_H */
