/*
    Copyright (C) 2020- TheTrustedComputer
*/

#include "mcts.h"

/*!
 *  @param _init    The MCTS node to initialize.
 *  @param _ancest  The ancestor or parent node.
 *  @param _descend The descendants or child nodes.
 *  @param _MOVE    The move that led to this node.
 *  @param _DEPTH   The tree depth of this node.
 */
void MCTSNode_initialize(MCTSNode *_init, MCTSNode *_ancest, MCTSNode *_descends, const uint8_t _MOVE)
{
    _init->ancestor = _ancest;
    _init->descendants = _descends;
    _init->points = _init->visits = _init->count = 0;
    _init->move = _MOVE;
}

void MCTSNode_destroy(MCTSNode *_destroyer)
{
    uint8_t n;
    
    if (_destroyer->descendants)
    {
        for (n = 0; n < _destroyer->count; ++n)
        {
            MCTSNode_destroy(&_destroyer->descendants[n]);
        }
        
        free(_destroyer->descendants);
    }
}

void MCTSNode_print(const MCTSNode *_PRINT)
{
    printf("\e[1m%d%c\e[0m: %lld %lld %d %.8Lf\n", _PRINT->move >> 4, 'A' + (_PRINT->move & 0xf), _PRINT->points, _PRINT->visits, _PRINT->count, MCTS_uct(_PRINT));
}

void MCTSNode_printAll(const MCTSNode *_PRINTALL, const int _DEPTH)
{
    uint8_t n;
    
    // Print all tree nodes
    if (_PRINTALL->count)
    {
        for (n = 0; n < _PRINTALL->count; ++n)
        {
            MCTSNode_printAll(_PRINTALL->descendants, _DEPTH + 1);
        }
    }
    else
    {
        printf("Depth %d: ", _DEPTH);
        MCTSNode_print(_PRINTALL);
    }
}

void MCTSNode_avgPoints(const MCTSNode *_PTS)
{
    long double avg;
    uint8_t tile, col, n;
    
    if (_PTS)
    {
        for (n = 0; n < _PTS->count; ++n)
        {
            avg = (long double)(_PTS->descendants[n].points) / _PTS->descendants[n].visits;
            tile = _PTS->descendants[n].move >> 4;
            col = _PTS->descendants[n].move & 0xf;
            printf("%d%c %.2Lf %lld %lld\n", tile, col + 'A', avg, _PTS->descendants[n].points, _PTS->descendants[n].visits);
        }
    }
}

long double MCTS_uct(const MCTSNode *_UCT)
{
    return _UCT->visits ? ((long double)(_UCT->points) / _UCT->visits) + (MCTS_UCT_C * sqrtl(logl((long double)(_UCT->ancestor->visits) / _UCT->visits))) : INFINITY;
}

/*!
 *  @param _selector The MCTS node to select.
 *  @param _ms       The Make 7 game state to update.
 *  @return          The selected MCTS node.
 */
MCTSNode *MCTS_select(MCTSNode *_selector, MakeSeven *_ms)
{
    MCTSNode *selected;
    long double bestUCT, currUCT;
    uint8_t n;
    
    // Check if this node has descendants
    if (_selector->count)
    {
        for (bestUCT = -INFINITY, n = 0; n < _selector->count; ++n)
        {
            // Check unvisited descendant nodes; select it for expansion
            if (!_selector->descendants[n].visits)
            {
                return _selector;
            }
            
            // Select the descendant with the highest UCT value
            if ((currUCT = MCTS_uct(&_selector->descendants[n])) > bestUCT)
            {
                bestUCT = currUCT;
                selected = &_selector->descendants[n];
            }
        }
        
        // Update the game state to select the next node
        MakeSeven_drop(_ms, selected->move >> 4, selected->move & 0xf);
        
        return MCTS_select(selected, _ms);
    }
    
    // This node has no descendants; expand it
    return _selector;
}

/*!
 *  @param _expander The MCTS node to expand.
 *  @param _MS       The Make 7 game state to generate moves from.
 *  @return          True if the expansion was successful, false otherwise.
 */
bool MCTS_expand(MCTSNode *_expander, const MakeSeven *_MS)
{
    MCTSNode expandee;
    uint8_t dropList[MAKESEVEN_SIZE_X3], n;
    
    // Check if the game is not over; do not expand if it is or already expanded
    if (!(MakeSeven_gameOver(_MS) || _expander->count))
    {
        // Generate the list of possible moves
        MakeSeven_generate(_MS, dropList, &_expander->count);
        
        // Reserve memory for descendant nodes
        if (!(_expander->descendants = malloc(sizeof(*_expander->descendants) * _expander->count)))
        {
            return false;
        }
        
        // Expand the nodes and add them to the list of descendants
        for (n = 0; n < _expander->count; ++n)
        {
            MCTSNode_initialize(&expandee, _expander, NULL, dropList[n]);
            _expander->descendants[n] = expandee;
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
long long MCTS_simulate(MakeSeven *_simulMS, const uint8_t _TURN)
{
    unsigned long long randMove;
    uint8_t simulMoves[MAKESEVEN_SIZE_X3], simulCount;
    
    // Play until the game is over
    for (;;)
    {
        // Get the list of possible drops to simulate
        MakeSeven_generate(_simulMS, simulMoves, &simulCount);
        
        // If there are moves, select one at random and play that move; otherwise, return no reward to indicate a draw
        if (simulCount)
        {
            randMove = genrand64_int64() % simulCount;
            MakeSeven_drop(_simulMS, simulMoves[randMove] >> 4, simulMoves[randMove] & 0xf);
            
            // If there is a win or a loss, return the score depending on the player who started the simulation
            if (MakeSeven_tilesSumToSeven(_simulMS))
            {
                return _TURN == (_simulMS->plyNum & 1) ? 1 : -1;
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
void MCTS_backpropagate(MCTSNode *_backpropagator, long long _simulResult)
{
    while (_backpropagator)
    {
        ++_backpropagator->visits;
        _backpropagator->points += _simulResult;
        _simulResult = -_simulResult;
        _backpropagator = _backpropagator->ancestor;
    }
}

/*!
 *  @param _root The root MCTS node to search from.
 *  @return      The move with the highest visits.
 */
MCTSResult MCTS_best(const MCTSNode *_root)
{
    MCTSNode best = *_root;
    long long currVisits, bestVisits = -1;
    uint8_t n;
    
    // Choose the node with the highest visits; it generally performs better than highest mean points per visit
    for (n = 0; n < _root->count; ++n)
    {
        if ((currVisits = _root->descendants[n].visits) > bestVisits)
        {
            bestVisits = currVisits;
            best = _root->descendants[n];
        }
    }
    
    return (MCTSResult) {(long double)(best.points) / best.visits, best.move};
}

/*!
 *  @param _MS       The Make 7 game state to search for a move.
 *  @param _WIN_HND  The handle to the console window (compilation for Windows only).
 *  @param _PARALLEL Use multithreading to speed up the search.
 *  @param _OUTPUT   Whether to output the search progress to the console.
 *  @return          The best move found by Monte Carlo tree search.
 */
uint8_t MCTS_search(const MakeSeven *_MS, const void *_WIN_HND, const bool _OUTPUT)
{
    
#if defined(_WIN64) || defined(_WIN32)
    HANDLE *winTerm = _WIN_HND;
#endif
    
    static MCTSNode root, *leaf;
    static MCTSResult result;
    
    time_t progress;
    double elapsed;
    long long i, secs, sims;
    long double oneTile[MAKESEVEN_SIZE], twoTile[MAKESEVEN_SIZE], threeTile[MAKESEVEN_SIZE];
    uint8_t tile, col;
    
    MakeSeven mctsMS = *_MS;
    
    // Catch SIGINT to stop the search
    signal(SIGINT, MCTS_stop);
    MCTSNode_initialize(&root, NULL, NULL, 0);
    
    for (progress = time(NULL), i = secs = 0; atomic_load(&runMCTS); ++i)
    {
        // Selection
        leaf = MCTS_select(&root, &mctsMS);
        
        // Expansion
        if (!MCTS_expand(leaf, &mctsMS))
        {
            // On no more memory, restore the game state and stop
            mctsMS = *_MS;
            break;
        }
        
        // Pick a random move to simulate and update the game state
        if (leaf->count)
        {
            leaf = &leaf->descendants[genrand64_int64() % leaf->count];
            MakeSeven_drop(&mctsMS, leaf->move >> 4, leaf->move & 0xf);
        }
        
        // Check immediate wins and losses, rewarding them with higher scores
        sims = MakeSeven_tilesSumToSeven(&mctsMS) ? (MAKESEVEN_AREA_P2 - mctsMS.plyNum) >> 1 : MCTS_simulate(&mctsMS, mctsMS.plyNum & 1);
        
        
        // Simulation and backpropagation
        MCTS_backpropagate(leaf, sims);
        
        // Reset the game state for the next loop
        mctsMS = *_MS;
        
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
        for (i = 0; i < MAKESEVEN_SIZE; ++i)
        {
            oneTile[i] = twoTile[i] = threeTile[i] = MCTS_INVALID;
        } 
        
        // Copy the points to the array
        for (i = 0; i < root.count; ++i)
        {
            tile = root.descendants[i].move >> 4;
            col = root.descendants[i].move & 0xf;
            
            switch (tile)
            {
            case 1:
                oneTile[col] = (long double)(root.descendants[i].points) / root.descendants[i].visits;
                break;
            case 2:
                twoTile[col] = (long double)(root.descendants[i].points) / root.descendants[i].visits;
                break;
            case 3:
                threeTile[col] = (long double)(root.descendants[i].points) / root.descendants[i].visits;
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

void MCTS_progress(const MCTSResult *_RESULT, const void *_WIN_HND, const unsigned long long _ITERS, const unsigned long long _SECS)
{
#if defined(_WIN64) || defined(_WIN32)
    HANDLE *winTerm = _WIN_HND;
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

void MCTS_pointStats(const void *_WIN_HND, const long double *_1T_PTS, const long double *_2T_PTS, const long double *_3T_PTS)
{
    int t, c;
    
#if defined(_WIN64) || defined(_WIN32)
    HANDLE *winTerm = _WIN_HND;
#endif
    
    for (t = 1; t <= 3; ++t)
    {
        printf("%d ", t);
        
        for (c = 0; c < MAKESEVEN_SIZE; ++c)
        {
            switch (t)
            {
            case 1:         
                if ((_1T_PTS[c] != MCTS_INVALID) && (_1T_PTS[c] <= -1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;31m");
#endif
                    
                }
                else if ((_1T_PTS[c] > -1.0l) && (_1T_PTS[c] < 1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;33m");
#endif
                    
                }
                else if (_1T_PTS[c] >= 1.0l)
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;32m");
#endif
                    
                }
                
#if defined(_WIN64) || defined(_WIN32)
                (_1T_PTS[c] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf ", _1T_PTS[c]);
#else
                (_1T_PTS[c] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf\e[0m ", _1T_PTS[c]);
#endif
                
                break;
            case 2:
                if ((_2T_PTS[c] != MCTS_INVALID) && (_2T_PTS[c] <= -1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;31m");
#endif
                    
                }
                else if ((_2T_PTS[c] > -1.0l) && (_2T_PTS[c] < 1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;33m");
#endif
                    
                }
                else if (_2T_PTS[c] >= 1.0l)
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;32m");
#endif
                    
                }
            
#if defined(_WIN64) || defined(_WIN32)
                (_2T_PTS[c] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf ", _2T_PTS[c]);
#else
                (_2T_PTS[c] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf\e[0m ", _2T_PTS[c]);
#endif
                
                break;
            case 3:
                if ((_3T_PTS[c] != MCTS_INVALID) && (_3T_PTS[c] <= -1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;31m");
#endif
                    
                }
                else if ((_3T_PTS[c] > -1.0l) && (_3T_PTS[c] < 1.0l))
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;33m");
#endif
                    
                }
                else if (_3T_PTS[c] >= 1.0l)
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(*winTerm, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                    printf("\e[1;32m");
#endif
                    
                }
            
#if defined(_WIN64) || defined(_WIN32)
                (_3T_PTS[c] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf ", _3T_PTS[c]);
#else
                (_3T_PTS[c] == MCTS_INVALID) ? printf("-- ") : printf("%.2Lf\e[0m ", _3T_PTS[c]);
#endif
            }    
        }
        
#if defined(_WIN64) || defined(_WIN32)
        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
        
        puts("");
    }
}

int MCTS_rootWorker(void *_mctsWorkArgs)
{
    MCTSRootThread *mrt = _mctsWorkArgs;
    MCTSNode *leaf;
    long long localSims;
    
    // Reseed the RNG with a unique seed for each thread
    init_genrand64(time(NULL) + clock() + mrt->id);
    
    // Run until the user stops us
    while (atomic_load(&runMCTS))
    {
        // Select
        leaf = MCTS_select(&mrt->localRoot, &mrt->copyMS);
        
        // Expand
        if (!MCTS_expand(leaf, &mrt->copyMS))
        {
            // On insufficient memory, stop the thread
            atomic_store(&runMCTS, false);
            return 1;
        }
        
        // Choose a random descendant and make that move
        if (leaf->count)
        {
            leaf = &leaf->descendants[genrand64_int64() % leaf->count];
            MakeSeven_drop(&mrt->copyMS, leaf->move >> 4, leaf->move & 0xf);
        }
        
        // Simulate and backpropagate
        localSims = MakeSeven_tilesSumToSeven(&mrt->copyMS) ? (MAKESEVEN_AREA_P2 - mrt->copyMS.plyNum) >> 1 : MCTS_simulate(&mrt->copyMS, mrt->copyMS.plyNum & 1);
        MCTS_backpropagate(leaf, localSims);
        
        // Reset the local game state for another iteration
        mrt->copyMS = *mrt->originMS;
        
        // Update the global root, locking it from other threads
        mtx_lock(mrt->gRootLock);
        MCTSNode_update(mrt->globalRoot, mrt->localRoot.descendants, mrt->totalGMoves);
        mtx_unlock(mrt->gRootLock);
        
        // Increment the number of iterations
        atomic_fetch_add(mrt->iters, 1);
    }
    
    return 0;
}

void MCTSNode_update(MCTSNode *_global, MCTSNode *_local, const uint8_t _G_MOVES)
{
    uint8_t n;
    
    for (n = 0; n < _G_MOVES; ++n)
    {
        _global[n].visits += _local[n].visits;
        _global[n].points += _local[n].points;
        
        /*
        // Detect an overflow of the visits counter
        if (_global[n].visits < 0)
        {
            // Set the run flag to false to stop all threads
            atomic_store(&runMCTS, false);
        }
        else
        {
             _global[n].visits += _local[n].visits;
        }
        
        // Points overflow
        if ((_global[n].points >= 0) && (_local[n].points > (INT64_MAX - _global[n].points)))
        {
            atomic_store(&runMCTS, false);
        }
        
        // Points underflow
        else if ((_global[n].points < 0) && (_local[n].points < (INT64_MIN - _global[n].points)))
        {
            atomic_store(&runMCTS, false);
        }
        else
        {
            _global[n].points += _local[n].points;
        }
        */
    }
}

uint8_t MCTS_rootParallel(MakeSeven *_MS, const void *_WIN_HND, const bool _OUTPUT)
{
#if defined(_WIN64) || defined(_WIN32)
    HANDLE *winTerm = _WIN_HND;
    thrCount = GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS);
#elifdef __unix__
    thrCount = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    
    thrd_t mctsThreads[thrCount];
    MCTSRootThread thrArgs[thrCount];
    MCTSNode thrLRoots[thrCount], bestRoot;
    MCTSResult thrBestRes;
    mtx_t rootLock;
    long double gOneTile[MAKESEVEN_SIZE], gTwoTile[MAKESEVEN_SIZE], gThreeTile[MAKESEVEN_SIZE];
    atomic_ullong i;
    unsigned long long secs;
    long long currVis, bestVis;
    struct timespec printTime;
    int t, thrRet, tile, col;
    uint8_t mvCount, list[MAKESEVEN_SIZE_X3];
    
    signal(SIGINT, MCTS_stop);
    MakeSeven_generate(_MS, list, &mvCount);
    atomic_init(&i, 0);
    printTime.tv_sec = 1;
    printTime.tv_nsec = 0;
    secs = 0;
    thrBestRes = (MCTSResult) {0.0l, 0};
    
    MCTSNode thrGRoot[mvCount];
    
    for (t = 0; t < mvCount; ++t)
    {
        MCTSNode_initialize(&thrGRoot[t], NULL, NULL, list[t]);
    }
    
    if (mtx_init(&rootLock, mtx_plain) != thrd_success)
    {
        fprintf(stderr, "Could not initialize the global root mutex.\n");
        exit(EXIT_FAILURE);
    }
    
    // Initialize thread arguments and create the threads
    for (t = 0; t < thrCount; ++t)
    {
        MCTSNode_initialize(&thrLRoots[t], NULL, NULL, 0);
        
        thrArgs[t] = (MCTSRootThread)
        {
            .copyMS = *_MS,
            .originMS = _MS,
            .localRoot = thrLRoots[t],
            .globalRoot = thrGRoot,
            .gRootLock = &rootLock,
            .iters = &i,
            .id = t,
            .totalGMoves = mvCount
        };
        
        switch (thrd_create(&mctsThreads[t], MCTS_rootWorker, &thrArgs[t]))
        {
        case thrd_nomem:
            fprintf(stderr, "Ran out of memory while creating thread #%d.\n", t);
            exit(EXIT_FAILURE);
        case thrd_error:
            fprintf(stderr, "Could not create thread #%d for root parallelization.\n", t);
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
        bestRoot = thrGRoot[0];
        bestVis = currVis = bestRoot.visits;
        
        for (t = 1; t < thrCount; ++t)
        {
            if ((currVis = thrGRoot[t].visits) > bestVis)
            {
                bestVis = currVis;
                bestRoot = thrGRoot[t];
            }
        }
        
        mtx_unlock(&rootLock);
        thrBestRes.bestMove = bestRoot.move;
        thrBestRes.meanPts = (long double)(bestRoot.points) / bestRoot.visits;
        
#if defined(_WIN64) || defined(_WIN32)
        MCTS_progress(&thrBestRes, winTerm, atomic_load(&i), ++secs);
#else
        MCTS_progress(&thrBestRes, NULL, atomic_load(&i), ++secs);
#endif
    
    }
        
    // Join the threads after completing the search
    for (t = 0; t < thrCount; ++t)
    {
        switch (thrd_join(mctsThreads[t], &thrRet))
        {
        case thrd_error:
            fprintf(stderr, "Could not join thread #%d after completing Monte Carlo tree search.\n", t);
            exit(EXIT_FAILURE);
        default:
            break;
        }
        
        MCTSNode_destroy(&thrLRoots[t]);
    }
    
    if (_OUTPUT)
    {
        // Same logic as the serial version to print the statistics
        for (t = 0; t < MAKESEVEN_SIZE; ++t)
        {
            gOneTile[t] = gTwoTile[t] = gThreeTile[t] = MCTS_INVALID;
        }
        
        for (t = 0; t < mvCount; ++t)
        {
            tile = thrGRoot[t].move >> 4;
            col = thrGRoot[t].move & 0xf;
            
            switch (tile)
            {
            case 1:
                gOneTile[col] = (long double)(thrGRoot[t].points) / thrGRoot[t].visits;
                break;
            case 2:
                gTwoTile[col] = (long double)(thrGRoot[t].points) / thrGRoot[t].visits;
                break;
            case 3:
                gThreeTile[col] = (long double)(thrGRoot[t].points) / thrGRoot[t].visits;
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

static inline void MCTS_stop(int _signal)
{
    atomic_store(&runMCTS, false);
}
