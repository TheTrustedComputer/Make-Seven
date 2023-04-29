/*
    Copyright (C) 2020- TheTrustedComputer
*/

#include "negamax.h"

void Negamax_setColumnMoveOrder(void)
{
    for (int i = 0; i < MAKESEVEN_SIZE; ++i) // Center to outermost
    {
        moveOrder[i] = MAKESEVEN_SIZE / 2 + (1 - 2 * (i & 1)) * (i + 1) / 2;
    }
}

bool Negamax_checkForSeven(const MakeSeven *_ms)
{
    MakeSeven ms = *_ms;
    uint8_t num, col;
    
    // Make all possible drops and check for sum of seven, then revert to original game state
    for (num = 4; --num;)
    {
        for (col = 0; col < MAKESEVEN_SIZE; ++col)
        {
            if (MakeSeven_drop(&ms, num, col))
            {
                if (MakeSeven_tilesSumToSeven(&ms))
                {
                    return true;
                }
                
                ms = *_ms;
            }
        }
    }
    
    return false;
}

int Negamax_search(MakeSeven *_ms, TranspositionTable *_tt, int _d, int _a, int _b)
{
    int rootScore, leafScore;
    uint8_t t, c;
    
    // Increment the number of game tree nodes searched
    ++nodes;
    
    // See if the score is in the transposition table
    if (abs((tableScore = TranspositionTable_loadVal(_tt, MakeSeven_hashEncode(_ms), _ms->tiles23[0], _ms->tiles23[1]))) >= NM_WIN)
    {
        return tableScore;
    }
    
    // Check for a "Make 7"
    if (Negamax_checkForSeven(_ms))
    {
        return NM_WIN; // The current player wins
    }
    
    // Check if the player cannot make any more moves or hiting maxiumum depth
    if (!_d || MakeSeven_hasNoMoreMoves(_ms))
    {
        return NM_DRAW; // Assume a draw
    }
    
    rootScore = _a;
    
    for (t = 4; --t;)
    {
        for (c = 0; c < MAKESEVEN_SIZE; ++c)
        {
            if (MakeSeven_drop(_ms, t, moveOrder[c]))
            {
                // Drop tiles and see if our score beats the current best score
                if ((leafScore = -Negamax_search(_ms, _tt, _d - 1, -_b, -_a)) > rootScore)
                {
                    rootScore = leafScore;
                }
                
                MakeSeven_undrop(_ms);
                
                // Update best score if it's better than the current best, and store lower bound
                if (rootScore > _a)
                {
                    TranspositionTable_storeVal(_tt, MakeSeven_hashEncode(_ms), _ms->tiles23[0], _ms->tiles23[1], (_a = rootScore));
                }
                
                // Alpha cut-off
                if (_a >= _b)
                {
                    return _a;
                }
            }
        }
    }
    
    // Save upper bound
    TranspositionTable_storeVal(_tt, MakeSeven_hashEncode(_ms), _ms->tiles23[0], _ms->tiles23[1], _a);

    return _a;
}

int Negamax_worker(void *_negamaxWorkArgs)
{
    NegamaxThread *nt = _negamaxWorkArgs;
    
    thrd_yield();
    mtx_lock(nt->startMtx);
    
    // Wait for the start barrier
    while (*nt->idle && (*nt->running < *nt->count))
    {
        cnd_wait(nt->startCnd, nt->startMtx);
    }
    
    // Solve this assigned position and set the result for this move
    mtx_unlock(nt->startMtx);
    nt->result = Negamax_solve(&nt->ms, nt->table, nt->verbose);
    Result_increment(&nt->result);
    nt->results[nt->move & 0xf] = nt->result;  
    mtx_lock(nt->finishMtx);
    
    // Let the main thread know that this thread has finished
    if (cnd_signal(nt->finishCnd) == thrd_success)
    {
        // Decrement the number of running threads and save this thread's ID
        --(*nt->running);
        *nt->finishID = nt->id;
    }
    
    mtx_unlock(nt->finishMtx);
        
    return 0;
}

Result Negamax_solve(MakeSeven *_ms, TranspositionTable *_tt, const bool _VERBOSE)
{
    int solution, depth, maxDepth = 49 - _ms->plyNum;
    
    // Iterative deepening to solve shallow wins and losses
    for (solution = NM_DRAW, depth = 0; depth < maxDepth; ++depth)
    {
        if (_VERBOSE)
        {
            printf("\rSolving...%d\r", depth);
#ifdef __unix__
            fflush(stdout);
#endif
        }
        
        if (abs((solution = Negamax_search(_ms, _tt, depth, -NM_WIN, NM_WIN))) >= NM_WIN)
        {
            return (Result) { solution > 0 ? WIN_CHAR : LOSS_CHAR, depth };
        }
    }
    
    return RESULT_DRAW;
}

Result Negamax_solveInParallel(MakeSeven *_ms, const bool _VERBOSE, Result *_r1, Result *_r2, Result *_r3, Result *_bestResl, uint8_t *_bestMove)
{
    int t, tileN, colN, runners, finishID;
    unsigned thrTableSize;
    Result bestResl;
    uint8_t dropList[MAKESEVEN_SIZE_X3], dropCount, nextUnsolved;
    bool idle;
    
    // Condition variables and mutexes for signaling
    mtx_t thrFinishMutex, thrStartMutex;
    cnd_t thrFinishCondV, thrStartCondV;
    
    // Generate all possible moves in the given position
    MakeSeven_generate(_ms, dropList, &dropCount);
    
    // Thread handles and arguments
    thrd_t thread[dropCount];
    NegamaxThread thrArgs[dropCount];
    bool winOnFirst[dropCount];
    TranspositionTable thrTT[dropCount];
    
    // Perform lazy thread creation; one drop move per thread
#if defined (_WIN64) || defined (_WIN32)
    if ((thrCount = GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS)) > dropCount)
    {
        thrCount = dropCount;
    }
#elifdef __unix__
    if ((thrCount = sysconf(_SC_NPROCESSORS_ONLN)) > dropCount)
    {
        thrCount = dropCount;
    }
#endif
    
    // Make it work with systems with low memory requirements
    if ((thrTableSize = TranspositionTable_prevprime((table.size + 1) / thrCount)) < TT_HASHSIZE)
    {
        thrTableSize = TT_HASHSIZE;
    }

    // Initialize the results with unknown values
    for (t = 0; t < MAKESEVEN_SIZE; ++t)
    {
        _r1[t].wdl = _r2[t].wdl = _r3[t].wdl = UNKNOWN_CHAR;
    }
    
    // Initialize game states, mutexes, condition variables, and results
    for (t = 0; t < dropCount; ++t)
    {
        thrArgs[t] = (NegamaxThread)
        { 
            .ms = *_ms,
            .count = &thrCount,
            .id = t,
            .running = &runners,
            .finishID = &finishID,
            .startMtx = &thrStartMutex,
            .finishMtx = &thrFinishMutex,
            .startCnd = &thrStartCondV,
            .finishCnd = &thrFinishCondV,
            .table = &thrTT[t],
            .move = dropList[t],
            .verbose = _VERBOSE,
            .idle = &idle
        };
        
        winOnFirst[t] = false;
        tileN = dropList[t] >> 4;
        MakeSeven_drop(&thrArgs[t].ms, tileN, dropList[t] & 0xf);
        
        // Assign the results array to the correct thread
        switch (tileN)
        {
        case 1:
            thrArgs[t].results = _r1;
            break;
        case 2:
            thrArgs[t].results = _r2;
            break;
        case 3:
            thrArgs[t].results = _r3;
        }
        
        // Search for a win on the first move and do not create a thread for it
        if (MakeSeven_tilesSumToSeven(&thrArgs[t].ms))
        {
            winOnFirst[t] = true;
        }
    }
    
    // Initialize the mutex and condition variable
    if (mtx_init(&thrStartMutex, mtx_plain) != thrd_success)
    {
        fprintf(stderr, "Could not initialize the mutex to start the negamax worker threads.\n");
        exit(EXIT_FAILURE);
    }
    
    if (mtx_init(&thrFinishMutex, mtx_plain) != thrd_success)
    {
        fprintf(stderr, "Could not initialize the mutex for the negamax worker threads.\n");
        exit(EXIT_FAILURE);
    }
    
    if (cnd_init(&thrStartCondV) != thrd_success)
    {
        fprintf(stderr, "Could not initialize the condition variable to start the negamax worker threads.\n");
        exit(EXIT_FAILURE);
    }
    
    if (cnd_init(&thrFinishCondV) != thrd_success)
    {
        fprintf(stderr, "Could not initialize the condition variable for the negamax worker threads.\n");
        exit(EXIT_FAILURE);
    }
    
    nextUnsolved = thrCount;
    runners = 0;
    idle = true;
    
    // Solve the position in parallel; each thread holds a copy of the game state to ensure no data races when making moves
    // However, the transposition table is unprotected, so threads may overwrite entries when one thread is still reading them
    // A brief debugging session shows no signs of a potential race condition, but it is certainly possible there is one
    // It is difficult to parallelize minimax with alpha-beta pruning effectively, as it is an inherently sequential algorithm
    for (t = 0; t < thrCount; ++t)
    {
        // Start the threads
        if (!winOnFirst[t])
        {
            if (!TranspositionTable_initialize(&thrTT[t], thrTableSize + 1))
            {
                fprintf(stderr, "Could not initialize the transposition table for thread #%d.\n", t);
                exit(EXIT_FAILURE);
            }
            
            switch (thrd_create(&thread[t], Negamax_worker, &thrArgs[t]))
            {
            case thrd_error:
                fprintf(stderr, "Could not create thread #%d.\n", t);
                exit(EXIT_FAILURE);
            case thrd_nomem:
                fprintf(stderr, "Could not allocate thread #%d.\n", t);
                exit(EXIT_FAILURE);
            default:
                ++runners;
                break;
            }
        }
    }
    
    // Signal all the threads to start solving their respective positions
    mtx_lock(&thrStartMutex);
    cnd_broadcast(&thrStartCondV);
    mtx_unlock(&thrStartMutex);
    idle = false;

    // Wait for all of the threads to finish
    while (runners)
    {
        // Check if a thread notified us it has finished solving
        mtx_lock(&thrFinishMutex);
        cnd_wait(&thrFinishCondV, &thrFinishMutex);
        mtx_unlock(&thrFinishMutex);
        
        // Print the finished thread's results
        printf("%d%c ", dropList[finishID] >> 4, 'A' + (dropList[finishID] & 0xf));
        Result_print(&thrArgs[finishID].result, _bestResl ? _bestResl : &thrArgs[finishID].result);
        puts("");
        
        // Reassign that thread to another unsolved move whenever possible
        if (nextUnsolved < dropCount)
        {
            thrArgs[finishID].ms = *_ms;
            MakeSeven_drop(&thrArgs[finishID].ms, dropList[nextUnsolved] >> 4, dropList[nextUnsolved] & 0xf);
            TranspositionTable_destroy(&thrTT[finishID]);
            
            if (!TranspositionTable_initialize(&thrTT[nextUnsolved], thrTableSize + 1))
            {
                fprintf(stderr, "Could not reinitialize the transposition table for thread #%d.\n", finishID);
                exit(EXIT_FAILURE);
            }
            
            switch (thrd_create(&thread[finishID], Negamax_worker, &thrArgs[nextUnsolved]))
            {
            case thrd_error:
                fprintf(stderr, "Could not reassign thread #%d to another unsolved move.\n", t);
                exit(EXIT_FAILURE);
            case thrd_nomem:
                fprintf(stderr, "Could not allocate thread #%d for reassignment to another unsolved move.\n", t);
                exit(EXIT_FAILURE);
            default:
                ++runners;
                ++nextUnsolved;
            }
        }
    }
    
    // Join the threads to the main thread
    for (t = 0; t < thrCount; ++t)
    {
        if (!winOnFirst[t])
        {
            TranspositionTable_destroy(&thrTT[t]);
            
            if (thrd_join(thread[t], NULL) == thrd_error)
            {
                fprintf(stderr, "Could not join negamax worker thread #%d to the main thread.\n", t);
            }
        }
    }
    
    // Look for immediate wins
    for (t = 0; t < dropCount; ++t)
    {
        if (winOnFirst[t])
        {
            colN = dropList[t] & 0xf;
            
            switch (dropList[t] >> 4)
            {
            case 1:
                _r1[colN].wdl = WIN_CHAR;
                _r1[colN].dt7 = 0;
                break;
            case 2:
                _r2[colN].wdl = WIN_CHAR;
                _r2[colN].dt7 = 0;
                break;
            case 3:
                _r3[colN].wdl = WIN_CHAR;
                _r3[colN].dt7 = 0;
            }
        }
    }
    
    if (!_bestResl) // Find the best result from the threads
    {
        bestResl = Result_getBestResult(_r1, _r2, _r3);
    }
    
    if (_bestMove) // Store the best move to the output parameter
    {
        *_bestMove = Result_getBestMove(_r1, _r2, _r3);
    }
    
    // Clean up mutexes and condition variables
    mtx_destroy(&thrFinishMutex);
    mtx_destroy(&thrStartMutex);
    cnd_destroy(&thrFinishCondV);
    cnd_destroy(&thrStartCondV);
    
    return _bestResl ? *_bestResl : bestResl;
}

void Negamax_getMoveScores(MakeSeven *_ms, Result *_r1, Result *_r2, Result *_r3, Result *_best)
{
    uint8_t t, c, cEnd;
    bool mirror;
    
    // Prevent null pointer deference
    if (_r1 && _r2 && _r3)
    {
        // Check to see if the mirror image of the game state is the same
        // If so, only search the left side of the grid
        mirror = MakeSeven_symmetrical(_ms);
        cEnd = mirror ? 4 : MAKESEVEN_SIZE;
        
        // Flush results with unknown values
        for (t = 0; t < cEnd; ++t)
        {
            _r1[t].wdl = _r2[t].wdl = _r3[t].wdl = UNKNOWN_CHAR;
        }
        
        // Loop through all possible drop moves and assign results
        for (t = 1; t <= 3; ++t)
        {
            printf("%d ", t);
            for (c = 0; c < cEnd; ++c)
            {
#ifdef __unix__
                fflush(stdout);
#endif                
                if (MakeSeven_drop(_ms, t, c))
                {
                    
                    // Look for immediate wins
                    if (MakeSeven_tilesSumToSeven(_ms))
                    {
                        switch (t)
                        {
                        case 1:
                            _r1[c].wdl = WIN_CHAR;
                            _r1[c].dt7 = 0;
                            
                            if (mirror)
                            {
                                _r1[MAKESEVEN_SIZE_M1 - c].wdl = WIN_CHAR;
                                _r1[MAKESEVEN_SIZE_M1 - c].dt7 = 0;
                            }
                            break;
                        case 2:
                            _r2[c].wdl = WIN_CHAR;
                            _r2[c].dt7 = 0;
                            
                            if (mirror)
                            {
                                _r2[MAKESEVEN_SIZE_M1 - c].wdl = WIN_CHAR;
                                _r2[MAKESEVEN_SIZE_M1 - c].dt7 = 0;
                            }
                            break;
                        case 3:
                            _r3[c].wdl = WIN_CHAR;
                            _r3[c].dt7 = 0;
                            
                            if (mirror)
                            {
                                _r3[MAKESEVEN_SIZE_M1 - c].wdl = WIN_CHAR;
                                _r3[MAKESEVEN_SIZE_M1 - c].dt7 = 0;
                            }
                        }
                    }
                    else
                    {
                        TranspositionTable_destroy(&table);
                        TranspositionTable_initialize(&table, table.size);
                        
                        // Solve for each move, as there is no win
                        switch (t)
                        {
                        case 1:
                            _r1[c] = Negamax_solve(_ms, &table, false);
                            Result_increment(&_r1[c]);
                            
                            if (mirror)
                            {
                                _r1[MAKESEVEN_SIZE_M1 - c] = _r1[c];
                            }
                            break;
                        case 2:
                            _r2[c] = Negamax_solve(_ms, &table, false);
                            Result_increment(&_r2[c]);
                            
                            if (mirror)
                            {
                                _r2[MAKESEVEN_SIZE_M1 - c] = _r2[c];
                            }
                            break;
                        case 3:
                            _r3[c] = Negamax_solve(_ms, &table, false);
                            Result_increment(&_r3[c]);
                            
                            if (mirror)
                            {
                                _r3[MAKESEVEN_SIZE_M1 - c] = _r3[c];
                            }
                        }
                    }
                    MakeSeven_undrop(_ms);
                }
                
                // Print the results
                switch (t)
                {
                case 1:
                    Result_print(&_r1[c], _best);
                    break;
                case 2:
                    Result_print(&_r2[c], _best);
                    break;
                case 3:
                    Result_print(&_r3[c], _best);
                }
            }
            
            // Print it for the right half if the grid is symmetrical
            for (; mirror && c < MAKESEVEN_SIZE; ++c)
            {
                switch (t)
                {
                case 1:
                    Result_print(&_r1[c], _best);
                    break;
                case 2:
                    Result_print(&_r2[c], _best);
                    break;
                case 3:
                    Result_print(&_r3[c], _best);
                }
            }
            puts("");
        }
    }
}
