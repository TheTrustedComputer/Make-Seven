/*
    Copyright (C) 2020- TheTrustedComputer
    
    Negamax is a variant of minimax to simplify the implementation of it, assuming the game is zero-sum: a win for one player is a loss for the other.
    min(a, b) = -max(-b, -a)
    
    Minimax is a depth-first search algorithm to find a move that maximizes the player's score.
    On the next turn, the opponent will minimize the score by negating it.
    
    However, minimax alone may not be enough to solve some fairly complex games like chess and checkers.
    An optimization called alpha-beta pruning cut branches of the tree if the current score is worse than the best score found so far.
    This saves time and nodes by not having to search the entire tree, as long there is good move ordering.
    A static move ordering technique, from center to edge, is used to improve the efficiency of alpha-beta.
    
    Even with alpha-beta pruning, there can be transpositions, move sequences that result in the same game state.
    By storing the scores of these game states to the transposition table, minimax avoids having to recompute them every time.
    This is done at the start before any search begins and saving the scores after the search is finished.
    
    To futher increase the performance of the algorithm, iterative deepening is used, where the depth is increased by one per iteration.
    It allows minimax to solve game states that have shallow wins or losses, but those deeper in the tree will take longer to solve.
*/

#ifndef NEGAMAX_H
#define NEGAMAX_H

#include <threads.h>
#include <stdatomic.h>

#include "make7.h"
#include "table.h"
#include "result.h"

// Enumeration for negamax scores
enum NegamaxScore
{
    NM_DRAW, NM_WIN
};

// Counter for the number of game tree nodes evaluated
static unsigned long long nodes;

// Array to hold the move order; the score obtained from the transposition table
static int moveOrder[MAKESEVEN_SIZE], tableScore;

// Transposition table object
static TranspositionTable table;

// The number of worker threads to use
static int thrCount;

// Negamax worker thread's parameters
typedef struct
{
    MakeSeven ms;
    int *count, id;
    atomic_int *running, *finishID;
    mtx_t *startMtx, *finishMtx;
    cnd_t *startCnd, *finishCnd;
    TranspositionTable *table;
    Result *results, result;
    uint8_t move;
    bool verbose;
    atomic_bool *idle;
}
NegamaxThread;

// Negamax
void Negamax_setColumnMoveOrder(void);                                                                  // Set up the move order for the columns
bool Negamax_checkForSeven(const MakeSeven*);                                                           // Helper function to check for a "Make 7"		
int Negamax_search(MakeSeven*, TranspositionTable*, int, int, int);                                                          // Do a negamax search on this position
int Negamax_worker(void*);                                                                              // Negamax worker thread's main function
Result Negamax_solve(MakeSeven*, TranspositionTable*, const bool);                                                           // Solve this game state and return the result
Result Negamax_solveInParallel(MakeSeven*, const bool, Result*, Result*, Result*, Result*, uint8_t*);   // Solve it using multiple threads
void Negamax_getMoveScores(MakeSeven*, Result*, Result*, Result*, Result*);                             // Get and print the results of all moves

#endif /* NEGAMAX_H */
