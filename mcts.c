/*
    Copyright (C) 2020- TheTrustedComputer
*/

#include "mcts.h"

// Toggle the run flag to stop after receiving SIGINT
static inline void MCTS_stop(int UNUSED)
{
    (void)(UNUSED); // To suppress the unused parameter warning
    atomic_store(&runMCTS, false);
}

/*!
 *  @param _init    The MCTS node to initialize.
 *  @param _ancest  The ancestor or parent node.
 *  @param _MOVE    The move that led to this node.
 *  @param _DEPTH   The tree depth of this node.
 */
void MCTSNode_initialize(MCTSNode* restrict _init, MCTSNode* restrict _ancest, const uint8_t _MOVE)
{
    _init->ancestor = _ancest;
    _init->descendant = NULL;
    _init->points = _init->visits = _init->count = 0;
    _init->move = _MOVE;
}

void MCTSNode_destroy(MCTSNode* restrict _dest)
{
    uint8_t node;
    
    if (_dest->count)
    {
        for (node = 0; node < _dest->count; node++)
        {
            MCTSNode_destroy(&_dest->descendant[node]);
        }
        
        free(_dest->descendant);
    }
}

void MCTSNode_printNode(const MCTSNode* restrict _PRINT)
{
    printf("\e[1m%d%c\e[0m: %lld %lld %d %.8Lf\n", _PRINT->move >> 4, 'A' + (_PRINT->move & 0xf), _PRINT->points, _PRINT->visits, _PRINT->count, MCTS_uct(_PRINT));
}

void MCTSNode_printAll(const MCTSNode* restrict _PRINTALL, const int _DEPTH)
{
    uint8_t node;
    
    // Print all tree nodes
    if (_PRINTALL->count)
    {
        for (node = 0; node < _PRINTALL->count; node++)
        {
            MCTSNode_printAll(_PRINTALL->descendant, _DEPTH + 1);
        }
    }
    else
    {
        printf("Depth %d: ", _DEPTH);
        MCTSNode_printNode(_PRINTALL);
    }
}

void MCTSNode_avgPoints(const MCTSNode* restrict _PTS)
{
    long double avg;
    uint8_t tile, col, node;
    
    for (node = 0; _PTS && (node < _PTS->count); node++)
    {
        avg = (long double)(_PTS->descendant[node].points) / _PTS->descendant[node].visits;
        tile = _PTS->descendant[node].move >> 4;
        col = _PTS->descendant[node].move & 0xf;
        printf("%d%c %.2Lf %lld %lld\n", tile, col + 'A', avg, _PTS->descendant[node].points, _PTS->descendant[node].visits);
    }
}

long double MCTS_uct(const MCTSNode* restrict _UCT)
{
    return ((long double)(_UCT->points) / _UCT->visits) + (/* MCTS_UCT_C * */ sqrtl(logl((long double)(_UCT->ancestor->visits) / _UCT->visits)));
}

/*!
 *  @param _selector The MCTS node to select.
 *  @param _m7       The Make 7 game state to update.
 *  @return          The selected MCTS node.
 */
MCTSNode *MCTS_select(MCTSNode* restrict _selector, Make7* restrict _m7)
{
    long double bestUCT, currUCT;
    MCTSNode *selected;
    uint8_t node;
    
    // As long as this node has descendants
    while (_selector->count)
    {
        for (bestUCT = -INFINITY, node = 0; node < _selector->count; node++)
        {
            // Check unvisited descendant nodes; select it for expansion
            if (!_selector->descendant[node].visits)
            {
                return _selector;
            }
            
            // Select the descendant with the highest UCT value
            if ((currUCT = MCTS_uct(&_selector->descendant[node])) > bestUCT)
            {
                bestUCT = currUCT;
                selected = &_selector->descendant[node];
            }
        }
        
        // Update the game state to select the next node
        Make7_drop(_m7, selected->move >> 4, selected->move & 0xf);
        _selector = selected;
    }
    
    // This node has no descendants; expand it
    return _selector;
}

/*!
 *  @param _expander The MCTS node to expand.
 *  @param _M7       The Make 7 game state to generate moves from.
 *  @return          True if the expansion was successful, false otherwise.
 */
bool MCTS_expand(MCTSNode* restrict _expander, const Make7* restrict _M7)
{
    MCTSNode expandee;
    uint8_t drops[MAKE7_SIZE_X3], node;
    
    // Check if the game is not over; do not expand if it is or already expanded
    if (!(_expander->count || Make7_gameOver(_M7)))
    {
        // Generate the list of possible moves
        Make7_generate(_M7, drops, &_expander->count);
        
        // Reserve memory for descendant nodes
        if (!(_expander->descendant = malloc(sizeof(*_expander->descendant) * _expander->count)))
        {
            return false;
        }
        
        // Expand the nodes and add them to the list of descendants
        for (node = 0; node < _expander->count; node++)
        {
            MCTSNode_initialize(&expandee, _expander, drops[node]);
            _expander->descendant[node] = expandee;
        }
    }
    
    return true;
}

/*!
 *  @param _simulMS The Make 7 game state to simulate.
 *  @param _TURN    Who's turn it is to play.
 *  @param _SCORE   The score to reward the simulation with.
 *  @return         The reward for the simulation.
 */
signed long long MCTS_simulate(Make7* restrict _simulMS, const uint8_t _TURN)
{
    unsigned long long randMove;
    uint8_t simulMoves[MAKE7_SIZE_X3], simulCount;
    
    // Play until the game is over
    for (;;)
    {
        // Get the list of possible drops to simulate
        Make7_generate(_simulMS, simulMoves, &simulCount);
        
        // If there are moves, select one at random and play that move; otherwise, return no reward to indicate a draw
        if (simulCount)
        {
            randMove = genrand64_int64() % simulCount;
            Make7_drop(_simulMS, simulMoves[randMove] >> 4, simulMoves[randMove] & 0xf);
            
            // If there is a win or a loss, return the score depending on the player who started the simulation
            if (Make7_tilesSumTo7(_simulMS))
            {
                // ^ is the same as != on a single bit
                return _TURN ^ (_simulMS->plyNum & 1) ? -1 : 1;
            }
        }
        else
        {
            return 0;
        }
    }
}

/*!
 *  @param _backpropagator The MCTS node to backpropagate from.
 *  @param _simulResult    The result of the simulation.
 */
void MCTS_backpropagate(MCTSNode* restrict _backpropagator, signed long long _simulResult)
{
    while (_backpropagator)
    {
        _backpropagator->visits++;
        _backpropagator->points += _simulResult;
        _simulResult = -_simulResult;
        _backpropagator = _backpropagator->ancestor;
    }
}

/*!
 *  @param _root The root MCTS node to search from.
 *  @return      The move with the highest visits.
 */
MCTSResult MCTS_best(MCTSNode* restrict _root)
{
    MCTSNode *best = &_root->descendant[0];
    unsigned long long currVisits, bestVisits = best->visits;
    uint8_t node;
    
    // Choose the node with the highest visits; it generally performs better than highest mean points per visit
    for (node = 1; node < _root->count; node++)
    {
        if ((currVisits = _root->descendant[node].visits) > bestVisits)
        {
            bestVisits = currVisits;
            best = &_root->descendant[node];
        }
    }
    
    return (MCTSResult) {(long double)(best->points) / best->visits, best->move};
}

/*!
 *  @param _M7       The Make 7 game state to search for a move.
 *  @param _WIN_HND  The handle to the console window (compilation for Windows only).
 *  @param _OUTPUT   Whether to output the search progress to the console.
 *  @return          The best move found by Monte Carlo tree search.
 */
uint8_t MCTS_search(const Make7* restrict _M7, void* restrict _WIN_HND, const bool _OUTPUT)
{
    
#if defined(_WIN64) || defined(_WIN32)
    HANDLE *winTerm = _WIN_HND;
#else
    (void)(_WIN_HND);
#endif
    
    static MCTSNode root, *leaf;
    static MCTSResult result;
    
    time_t progress;
    double elapsed;
    long long i, secs, sims;
    long double oneTile[MAKE7_SIZE], twoTile[MAKE7_SIZE], threeTile[MAKE7_SIZE];
    uint8_t tile, col;
    
    Make7 mctsM7 = *_M7;
    
    // Catch SIGINT to stop the search
    signal(SIGINT, MCTS_stop);
    MCTSNode_initialize(&root, NULL, 0);
    
    for (progress = time(NULL), i = secs = 0; atomic_load(&runMCTS); i++)
    {
        // Selection
        leaf = MCTS_select(&root, &mctsM7);
        
        // Expansion
        if (!MCTS_expand(leaf, &mctsM7))
        {
            // On no more memory, restore the game state and stop
            mctsM7 = *_M7;
            break;
        }
        
        // Pick a random move to simulate and update the game state
        if (leaf->count)
        {
            leaf = &leaf->descendant[genrand64_int64() % leaf->count];
            Make7_drop(&mctsM7, leaf->move >> 4, leaf->move & 0xf);
        }
        
        // Check immediate wins and losses, rewarding them with higher scores
        sims = Make7_tilesSumTo7(&mctsM7) ? MAKE7_AREA_P1 - mctsM7.plyNum : MCTS_simulate(&mctsM7, mctsM7.plyNum & 1);
        
        // Simulation and backpropagation
        MCTS_backpropagate(leaf, sims);
        
        // Reset the game state for the next loop
        mctsM7 = *_M7;
        
        // Compute best move found and print the progress
        if ((elapsed = difftime(time(NULL), progress)) >= 1)
        {
            result = MCTS_best(&root);
            
#if defined(_WIN64) || defined(_WIN32)
            MCTS_progress(&result, winTerm, i, ++secs);
#else
            MCTS_progress(&result, NULL, i, ++secs);
#endif
            
            progress = time(NULL);
        }
    }
    
    // Get the best move by average points per visit
    result = MCTS_best(&root);
    printf("\a\n");
    
    if (_OUTPUT)
    {
        // Initialize the array of points for invalid moves
        for (i = 0; i < MAKE7_SIZE; i++)
        {
            oneTile[i] = twoTile[i] = threeTile[i] = MCTS_INVALID;
        }
        
        // Copy the points to the array
        for (i = 0; i < root.count; i++)
        {
            tile = root.descendant[i].move >> 4;
            col = root.descendant[i].move & 0xf;
            
            switch (tile)
            {
            case 1:
                oneTile[col] = (long double)(root.descendant[i].points) / root.descendant[i].visits;
                break;
            case 2:
                twoTile[col] = (long double)(root.descendant[i].points) / root.descendant[i].visits;
                break;
            case 3:
                threeTile[col] = (long double)(root.descendant[i].points) / root.descendant[i].visits;
            }
        }
        
        // Print the point statistics by tile and column
#if defined(_WIN64) || defined(_WIN32)
        MCTS_pointStats(winTerm, oneTile, twoTile, threeTile);
#else
        MCTS_pointStats(NULL, oneTile, twoTile, threeTile);
#endif
    }
    
    signal(SIGINT, SIG_DFL);
    MCTSNode_destroy(&root);
    atomic_store(&runMCTS, true);
    
    return result.bestMove;
}

void MCTS_progress(const MCTSResult* restrict _RESULT, void* restrict _WIN_HND, const unsigned long long _ITERS, const unsigned long long _SECS)
{
#if defined(_WIN64) || defined(_WIN32)
    HANDLE *winTerm = _WIN_HND;
#else
    (void)(_WIN_HND);
#endif
    
    printf("\r");
    
    if (_RESULT->meanPts <= -1.0l)
    {
        
#if defined(_WIN64) || defined(_WIN32)
        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
        printf("\e[1;31m");
#endif
        
    }
    else if ((_RESULT->meanPts > -1.0l) && (_RESULT->meanPts < 1.0l))
    {
        
#if defined(_WIN64) || defined(_WIN32)
        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
        printf("\e[1;33m");
#endif
    
    }
    else
    {
        
#if defined(_WIN64) || defined(_WIN32)
        SetConsoleTextAttribute(*winTerm, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
        printf("\e[1;32m");
#endif
        
    }
    
#if defined(_WIN64) || defined(_WIN32)
    printf("%d%c %.3Lf ", (_RESULT->bestMove >> 4), 'A' + (_RESULT->bestMove & 0xf), _RESULT->meanPts);
    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    printf("%llu %llu %llu", _ITERS, (_ITERS / _SECS), _SECS);
#else
    printf("%d%c %.3Lf\e[0m %llu %llu %llu", (_RESULT->bestMove >> 4), 'A' + (_RESULT->bestMove & 0xf), _RESULT->meanPts, _ITERS, (_ITERS / _SECS), _SECS);
#endif
    
#ifdef __unix__
    fflush(stdout);
#endif
    
}

void MCTS_pointStats(void* restrict _WIN_HND, const long double* restrict _1T_PTS, const long double* restrict _2T_PTS, const long double* restrict _3T_PTS)
{
    int tile, col;
    
#if defined(_WIN64) || defined(_WIN32)
    HANDLE *winTerm = _WIN_HND;
#else
    (void)(_WIN_HND);
#endif
    
    for (tile = 1; tile <= 3; tile++)
    {
        printf("%d ", tile);
        
        for (col = 0; col < MAKE7_SIZE; col++)
        {
            switch (tile)
            {
            case 1:         
                if ((_1T_PTS[col] != MCTS_INVALID) && (_1T_PTS[col] <= -1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;31m");
#endif
                    
                }
                else if ((_1T_PTS[col] > -1.0l) && (_1T_PTS[col] < 1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;33m");
#endif
                    
                }
                else if (_1T_PTS[col] >= 1.0l)
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;32m");
#endif
                    
                }
                
#if defined(_WIN64) || defined(_WIN32)
                (_1T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf ", _1T_PTS[col]);
#else
                (_1T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf\e[0m ", _1T_PTS[col]);
#endif
                
                break;
            case 2:
                if ((_2T_PTS[col] != MCTS_INVALID) && (_2T_PTS[col] <= -1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;31m");
#endif
                    
                }
                else if ((_2T_PTS[col] > -1.0l) && (_2T_PTS[col] < 1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;33m");
#endif
                    
                }
                else if (_2T_PTS[col] >= 1.0l)
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;32m");
#endif
                    
                }
            
#if defined(_WIN64) || defined(_WIN32)
                (_2T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf ", _2T_PTS[col]);
#else
                (_2T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf\e[0m ", _2T_PTS[col]);
#endif
                
                break;
            case 3:
                if ((_3T_PTS[col] != MCTS_INVALID) && (_3T_PTS[col] <= -1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;31m");
#endif
                    
                }
                else if ((_3T_PTS[col] > -1.0l) && (_3T_PTS[col] < 1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;33m");
#endif
                    
                }
                else if (_3T_PTS[col] >= 1.0l)
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;32m");
#endif
                    
                }
            
#if defined(_WIN64) || defined(_WIN32)
                (_3T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf ", _3T_PTS[col]);
#else
                (_3T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf\e[0m ", _3T_PTS[col]);
#endif
            }
        }
        
#if defined(_WIN64) || defined(_WIN32)
        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
        
        puts("");
    }
}

int MCTS_rootWorker(void *_args)
{
    MCTSRootThread *mrt = _args;
    MCTSNode *leaf;
    long long localSim;
    
    // Reseed the RNG with a unique seed for each thread
    init_genrand64(time(NULL) + clock() + mrt->id);
    
    // Run until the user stops us
    while (atomic_load(&runMCTS))
    {
        // Select
        leaf = MCTS_select(&mrt->localRoot, &mrt->copyM7);
        
        // Expand
        if (!MCTS_expand(leaf, &mrt->copyM7))
        {
            // On insufficient memory, stop the thread
            atomic_store(&runMCTS, false);
            return 1;
        }
        
        // Choose a random descendant and make that move
        if (leaf->count)
        {
            leaf = &leaf->descendant[genrand64_int64() % leaf->count];
            Make7_drop(&mrt->copyM7, leaf->move >> 4, leaf->move & 0xf);
        }
        
        // Simulate and backpropagate
        localSim = Make7_tilesSumTo7(&mrt->copyM7) ? MAKE7_AREA_P1 - mrt->copyM7.plyNum : MCTS_simulate(&mrt->copyM7, mrt->copyM7.plyNum & 1);
        MCTS_backpropagate(leaf, localSim);
        
        // Reset the local game state for another iteration
        mrt->copyM7 = *mrt->originM7;
        
        // Update the global root, locking it from other threads
        mtx_lock(mrt->gRootLock);
        MCTSNode_update(mrt->globalRoot, mrt->localRoot.descendant, mrt->totalGMoves);
        mtx_unlock(mrt->gRootLock);
        
        // Increment the number of iterations
        atomic_fetch_add(mrt->iters, 1);
    }
    
    return 0;
}

void MCTSNode_update(MCTSNode* restrict _global, MCTSNode* restrict _local, const uint8_t _G_MOVES)
{
    uint8_t node;
    
    for (node = 0; node < _G_MOVES; node++)
    {
        _global[node].visits += _local[node].visits;
        _global[node].points += _local[node].points;
    }
}

uint8_t MCTS_rootParallel(Make7* restrict _M7, void* restrict _WIN_HND, const bool _OUTPUT)
{
#if defined(_WIN64) || defined(_WIN32)
    HANDLE *winTerm = _WIN_HND;
    thrCount = GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS);
#elifdef __unix__
    (void)(_WIN_HND);
    thrCount = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    
    thrd_t mctsThreads[thrCount];
    MCTSRootThread thrArgs[thrCount];
    MCTSNode thrLRoots[thrCount], *bestRoot;
    MCTSResult thrBestRes;
    mtx_t rootLock;
    long double gOneTile[MAKE7_SIZE], gTwoTile[MAKE7_SIZE], gThreeTile[MAKE7_SIZE];
    atomic_ullong i;
    unsigned long long secs, currVis, bestVis;
    struct timespec printTime;
    int thr, thrRet, tile, col;
    uint8_t mvCount, list[MAKE7_SIZE_X3];
    
    signal(SIGINT, MCTS_stop);
    Make7_generate(_M7, list, &mvCount);
    atomic_init(&i, 0);
    printTime.tv_sec = 1;
    printTime.tv_nsec = 0;
    secs = 0;
    thrBestRes = (MCTSResult) {0.0l, 0};
    
    MCTSNode thrGRoot[mvCount];
    
    for (thr = 0; thr < mvCount; thr++)
    {
        MCTSNode_initialize(&thrGRoot[thr], NULL, list[thr]);
    }
    
    if (mtx_init(&rootLock, mtx_plain) != thrd_success)
    {
        fprintf(stderr, "Could not initialize the global root mutex.\n");
        exit(EXIT_FAILURE);
    }
    
    // Initialize thread arguments and create the threads
    for (thr = 0; thr < thrCount; thr++)
    {
        MCTSNode_initialize(&thrLRoots[thr], NULL, 0);
        
        thrArgs[thr] = (MCTSRootThread)
        {
            .copyM7 = *_M7,
            .originM7 = _M7,
            .localRoot = thrLRoots[thr],
            .globalRoot = thrGRoot,
            .gRootLock = &rootLock,
            .iters = &i,
            .id = thr,
            .totalGMoves = mvCount
        };
        
        switch (thrd_create(&mctsThreads[thr], MCTS_rootWorker, &thrArgs[thr]))
        {
        case thrd_nomem:
            fprintf(stderr, "Ran out of memory while creating thread #%d.\n", thr);
            exit(EXIT_FAILURE);
        case thrd_error:
            fprintf(stderr, "Could not create thread #%d for root parallelization.\n", thr);
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }
    
    // Print the progress of the search every second until the user interrupts
    while (atomic_load(&runMCTS))
    {
        thrd_sleep(&printTime, NULL);
        mtx_lock(&rootLock);
        bestRoot = &thrGRoot[0];
        bestVis = currVis = bestRoot->visits;
        
        for (thr = 1; thr < thrCount; thr++)
        {
            if ((currVis = thrGRoot[thr].visits) > bestVis)
            {
                bestVis = currVis;
                bestRoot = &thrGRoot[thr];
            }
        }
        
        thrBestRes.bestMove = bestRoot->move;
        thrBestRes.meanPts = (long double)(bestRoot->points) / bestRoot->visits;
        mtx_unlock(&rootLock);
        
#if defined(_WIN64) || defined(_WIN32)
        MCTS_progress(&thrBestRes, winTerm, atomic_load(&i), ++secs);
#else
        MCTS_progress(&thrBestRes, NULL, atomic_load(&i), ++secs);
#endif
    
    }
    
    // Join the threads after completing the search
    for (thr = 0; thr < thrCount; thr++)
    {
        switch (thrd_join(mctsThreads[thr], &thrRet))
        {
        case thrd_error:
            fprintf(stderr, "Could not join thread #%d after completing Monte Carlo tree search.\n", thr);
            exit(EXIT_FAILURE);
        default:
            break;
        }
        
        MCTSNode_destroy(&thrLRoots[thr]);
    }
    
    if (_OUTPUT)
    {
        // Same logic as the serial version to print the statistics
        for (thr = 0; thr < MAKE7_SIZE; thr++)
        {
            gOneTile[thr] = gTwoTile[thr] = gThreeTile[thr] = MCTS_INVALID;
        }
        
        for (thr = 0; thr < mvCount; thr++)
        {
            tile = thrGRoot[thr].move >> 4;
            col = thrGRoot[thr].move & 0xf;
            
            switch (tile)
            {
            case 1:
                gOneTile[col] = (long double)(thrGRoot[thr].points) / thrGRoot[thr].visits;
                break;
            case 2:
                gTwoTile[col] = (long double)(thrGRoot[thr].points) / thrGRoot[thr].visits;
                break;
            case 3:
                gThreeTile[col] = (long double)(thrGRoot[thr].points) / thrGRoot[thr].visits;
            }
        }
        
        puts("\a");
        
#if defined(_WIN64) || defined(_WIN32)
        MCTS_pointStats(winTerm, gOneTile, gTwoTile, gThreeTile);
#else
        MCTS_pointStats(NULL, gOneTile, gTwoTile, gThreeTile);
#endif
    }

    signal(SIGINT, SIG_DFL);
    atomic_store(&runMCTS, true);
    mtx_destroy(&rootLock);
    
    return thrBestRes.bestMove;
}
