/*
    Copyright (C) 2023- TheTrustedComputer
    
    Monte Carlo tree search for Make 7.
    
    A popular game-playing algorithm not unlike minimax but uses random simulated games to determine the best move.
    Simple as 1-2-3-4: select, expand, simulate, and backpropagate. Repeat until either timeout or reaching memory limit.
    
    This implementation uses a points-based reward system than a naive win-rate statistic, encouraging a stronger level of play.
    Wins are worth +1 point, losses -1 point, and draws 0 points. Immediate wins or losses are worth one plus the game grid's area minus the depth.
    To do this, an extra field called "depth" for the tree depth is used to determine the score of the simulation, starting at zero.
    It is still not immune to shallow traps; a good short-term strategy becomes a bad long-term strategy; this is a weakness of random sampling.
    
    Despite the weaknesses of Monte Carlo tree search, it is still a very useful for three reasons:
    Best suited for games with large branching factors where traditional minimax becomes impractical to use.
    No evaluation function is necessary; the algorithm learns from results of the simulations.
    Highly parallelizable! It is difficult to parallelize minimax due to its inherently serial nature.
    
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
#define MCTS_INVALID -100.0l // Valid range is [-50, 50]

// Flag to run the search via Ctrl-C
static atomic_bool runMCTS = true;

// The data structure housing a Monte Carlo tree search node
// The game is not stored to reduce memory footprint when running for long periods
// Rather, it is updated as the algorithm progresses
typedef struct MCTSNode
{
    long long points, visits;
    struct MCTSNode *ancestor, *descendants;
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
    MakeSeven copyMS;
    MCTSNode localRoot, *globalRoot;
    MakeSeven *originMS;
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
    MakeSeven simulMS;
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
void MCTSNode_initialize(MCTSNode*, MCTSNode*, MCTSNode*, const uint8_t);                                       // Initialize a Monte Carlo tree search node
void MCTSNode_destroy(MCTSNode*);                                                                               // Recursively release memory from it and its descendants

// Functions for debugging
void MCTSNode_print(const MCTSNode*);                                                                           // Print this node's current information
void MCTSNode_printAll(const MCTSNode*, const int);                                                             // Print the nodes of the tree
void MCTSNode_avgPoints(const MCTSNode*);                                                                       // Print the average point per visits of descendant nodes

// Monte Carlo tree search
long double MCTS_uct(const MCTSNode*);                                                                          // Compute the upper confidence bound for trees
MCTSNode *MCTS_select(MCTSNode*, MakeSeven*);                                                                   // Select the best node to expand
bool MCTS_expand(MCTSNode*, const MakeSeven*);                                                                  // Expand the selected node
long long MCTS_simulate(MakeSeven*, const uint8_t);                                                             // Simulate a random game from the current state
void MCTS_backpropagate(MCTSNode*, long long);                                                                  // Backpropagate the result of the simulation
MCTSResult MCTS_best(const MCTSNode*);                                                                          // Return the best move from the root node
uint8_t MCTS_search(const MakeSeven*, const void*, const bool);                                                 // Entry point for the Monte Carlo tree search algorithm
static inline void MCTS_stop(int);                                                                              // Toggle the run flag to stop after receiving SIGINT
void MCTS_progress(const MCTSResult*, const void*, const unsigned long long, const unsigned long long);
void MCTS_pointStats(const void*, const long double*, const long double*, const long double*);

// Multi-threading
void MCTSNode_update(MCTSNode*, MCTSNode*, const uint8_t);
int MCTS_rootWorker(void*);
uint8_t MCTS_rootParallel(MakeSeven*, const void*, const bool);

#endif /* MCTS_H */
