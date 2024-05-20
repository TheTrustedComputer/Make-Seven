/*
    Copyright (C) 2020- TheTrustedComputer
*/

#include "negamax.h"

void Negamax_setColumnMoveOrder(void)
{
    for (int i = 0; i < MAKE7_SIZE; i++) // Center to outermost
    {
        moveOrder[i] = (MAKE7_SIZE >> 1) + (1 - 2 * (i & 1)) * ((i + 1) >> 1);
    }
}

int Negamax_search(const Make7* restrict _M7, TransTable* restrict _tt, const int  _D, int _a, int _b)
{    
    // Increment the number of game tree nodes searched
    atomic_fetch_add(&nodes, 1);
    
    // See if the score is in the transposition table
    if ((tableScore = TransTable_load(_tt, Make7_hashEncode(_M7), _M7->tiles23[0], _M7->tiles23[1])))
    {
        return tableScore;
    }
    
    // Check for a "Make 7"
    if (Make7_checkFor7(_M7))
    {
        return NM_WIN; // The current player wins
    }
    
    // Check if the player cannot make any more moves or hiting maxiumum depth
    if (!_D || Make7_noMoreMoves(_M7))
    {
        return NM_DRAW; // Assume a draw
    }
    
    int leafScore, rootScore = _a;
    Make7 negamaxM7 = *_M7;
    
    for (uint8_t tile = 4; --tile;)
    {
        for (uint8_t col = 0; col < MAKE7_SIZE; col++)
        {
            if (Make7_drop(&negamaxM7, tile, moveOrder[col]))
            {
                // Drop tiles and see if our score beats the current best score
                if ((leafScore = -Negamax_search(&negamaxM7, _tt, _D - 1, -_b, -_a)) > rootScore)
                {
                    rootScore = leafScore;
                }
                
                negamaxM7 = *_M7;
                
                // Update best score if it's better than the current best, and store the lower bound
                if (_a < rootScore)
                {
                    TransTable_store(_tt, Make7_hashEncode(_M7), _M7->tiles23[0], _M7->tiles23[1], (_a = rootScore));
                    
                    // Alpha cut-off
                    if (_a >= _b)
                    {
                        return _a;
                    }
                }
            }
        }
    }
    
    // Save the upper bound
    TransTable_store(_tt, Make7_hashEncode(_M7), _M7->tiles23[0], _M7->tiles23[1], _a);

    return _a;
}

int Negamax_worker(void *_args)
{
    NegamaxArgs *nt = _args;
    
    mtx_lock(nt->startMtx);
    
    // Wait for the start barrier
    while (atomic_load(nt->idle))
    {
        cnd_wait(nt->startCnd, nt->startMtx);
    }
    
    // Solve this assigned position and set the result for this move
    mtx_unlock(nt->startMtx);
    nt->result = Negamax_solve(&nt->m7, nt->table, nt->verbose);
    Result_increment(&nt->result);
    nt->results[nt->move & 0xf] = nt->result;
    mtx_lock(nt->finishMtx);
    
    // Let the main thread know that this thread has finished
    if (cnd_signal(nt->finishCnd) == thrd_success)
    {
        // Decrement the number of running threads and save this thread's ID
        atomic_fetch_sub(nt->running, 1);
        atomic_store(nt->finishID, nt->id);
        atomic_store(nt->solved, true);
    }
    
    mtx_unlock(nt->finishMtx);
    
    return 0;
}

Result Negamax_solve(Make7* restrict _m7, TransTable* restrict _tt, const bool _VERBOSE)
{
    int solution = NM_DRAW, maxDep = MAKE7_AREA - Make7_plyNum(_m7);
    
    // Iterative deepening to solve shallow wins and losses
    for (int depth = 0; depth < maxDep; depth++)
    {
        if (_VERBOSE)
        {
            printf("\rSolving...%d\r", depth);
#ifdef __unix__
            fflush(stdout);
#endif
        }
        
        if (abs((solution = Negamax_search(_m7, _tt, depth, -NM_WIN, NM_WIN))) >= NM_WIN)
        {
            return (Result) { solution > 0 ? WIN_CHAR : LOSS_CHAR, depth };
        }
    }
    
    return RESULT_DRAW;
}

Result Negamax_solve_parallel(Make7* restrict _m7, const bool _VERBOSE, Result *_r1, Result *_r2, Result *_r3, Result *_bestResl, uint8_t *_bestMove)
{
    int thr, tileN, colN, finished, terminals;
    atomic_int thrRunners, finishTID;
    unsigned thrTableSize;
    Result bestResl;
    uint8_t dropList[MAKE7_SIZE_X3], dropCount, nextUnsolved;
    atomic_bool idle, thrSolved;
    mtx_t thrFinishMutex, thrStartMutex;
    cnd_t thrFinishCondV, thrStartCondV;
    
    // Generate all possible moves in the given position
    Make7_generate(_m7, dropList, &dropCount);
    
    // Thread handles and arguments
    thrd_t thread[dropCount];
    NegamaxArgs thrArgs[dropCount];
    bool winOnFirst[dropCount];
    TransTable thrTT[dropCount];
    
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
    if ((thrTableSize = TransTable_prevprime((table.size + 1) / thrCount)) <= 3)
    {
        thrTableSize = TT_HASHSIZE;
    }

    // Initialize the results with unknown values
    for (thr = 0; thr < MAKE7_SIZE; thr++)
    {
        _r1[thr].wdl = _r2[thr].wdl = _r3[thr].wdl = UNKNOWN_CHAR;
    }
    
    // Initialize game states, mutexes, condition variables, and results
    for (terminals = thr = 0; thr < dropCount; thr++)
    {
        thrArgs[thr] = (NegamaxArgs)
        {
            .m7 = *_m7,
            .running = &thrRunners,
            .finishID = &finishTID,
            .idle = &idle,
            .solved = &thrSolved,
            .startMtx = &thrStartMutex,
            .finishMtx = &thrFinishMutex,
            .startCnd = &thrStartCondV,
            .finishCnd = &thrFinishCondV,
            .table = &thrTT[thr],
            .id = thr,
            .move = dropList[thr],
            .verbose = _VERBOSE
        };
        
        winOnFirst[thr] = false;
        tileN = dropList[thr] >> 4;
        Make7_drop(&thrArgs[thr].m7, tileN, dropList[thr] & 0xf);
        
        // Assign the results array to the correct thread
        switch (tileN)
        {
        case 1:
            thrArgs[thr].results = _r1;
            break;
        case 2:
            thrArgs[thr].results = _r2;
            break;
        case 3:
            thrArgs[thr].results = _r3;
        }
        
        // Search for a win on the first move and do not create a thread for it
        if (Make7_tilesSumTo7(&thrArgs[thr].m7))
        {
            winOnFirst[thr] = true;
            terminals++;
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
    
    nextUnsolved = thrCount - terminals;
    atomic_init(&thrRunners, 0);
    atomic_init(&thrSolved, false);
    atomic_init(&idle, true);
    
    // Solve the position in parallel; each thread holds a copy of the game state to ensure no data races when making moves
    // It is difficult to parallelize minimax with alpha-beta pruning effectively, as it is an inherently sequential algorithm
    for (thr = 0; thr < thrCount; thr++)
    {
        // Start the threads
        if (!winOnFirst[thr])
        {
            if (!TransTable_initialize(&thrTT[thr], thrTableSize + 1))
            {
                fprintf(stderr, "Could not initialize the transposition table for thread #%d.\n", thr);
                exit(EXIT_FAILURE);
            }
            
            switch (thrd_create(&thread[thr], Negamax_worker, &thrArgs[thr]))
            {
            case thrd_error:
                fprintf(stderr, "Could not create thread #%d.\n", thr);
                exit(EXIT_FAILURE);
            case thrd_nomem:
                fprintf(stderr, "Could not allocate thread #%d.\n", thr);
                exit(EXIT_FAILURE);
            default:
                atomic_fetch_add(&thrRunners, 1);
                break;
            }
        }
    }
    
    // 
    atomic_store(&idle, false);
    
    // Signal the worker threads to continue after the barrier
    //mtx_lock(&thrStartMutex);
    cnd_broadcast(&thrStartCondV);
    //mtx_unlock(&thrStartMutex);
    
    // Wait for the threads to finish
    while (atomic_load(&thrRunners))
    {
        mtx_lock(&thrFinishMutex);
        
        // Check if a thread notified us it has finished solving
        if (atomic_load(&thrSolved))
        {
            // Print the finished thread's results
            finished = atomic_load(&finishTID);
            printf("%d%c ", dropList[finished] >> 4, 'A' + (dropList[finished] & 0xf));
            Result_print(&thrArgs[finished].result, _bestResl ? _bestResl : &thrArgs[finished].result);
            puts("");
            
            // Reassign that thread to another unsolved move whenever possible
            if (nextUnsolved < dropCount)
            {
                thrArgs[finished].m7 = *_m7;
                Make7_drop(&thrArgs[finished].m7, dropList[nextUnsolved] >> 4, dropList[nextUnsolved] & 0xf);
                TransTable_destroy(&thrTT[finished]);
                
                if (!TransTable_initialize(&thrTT[nextUnsolved], thrTableSize + 1))
                {
                    fprintf(stderr, "Could not reinitialize the transposition table for thread #%d.\n", finished);
                    exit(EXIT_FAILURE);
                }
                
                switch (thrd_create(&thread[finished], Negamax_worker, &thrArgs[nextUnsolved]))
                {
                case thrd_error:
                    fprintf(stderr, "Could not reassign thread #%d to another unsolved move.\n", thr);
                    exit(EXIT_FAILURE);
                case thrd_nomem:
                    fprintf(stderr, "Could not allocate thread #%d for reassignment to another unsolved move.\n", thr);
                    exit(EXIT_FAILURE);
                default:
                    atomic_fetch_add(&thrRunners, 1);
                    nextUnsolved++;
                }
            }
            
            atomic_store(&thrSolved, false);
        }
        else
        {
            // Wait for the signal from the threads
            cnd_wait(&thrFinishCondV, &thrFinishMutex);
        }
        
        mtx_unlock(&thrFinishMutex);
    }
    
    // Join the threads to the main thread
    for (thr = 0; thr < thrCount; thr++)
    {
        if (!winOnFirst[thr])
        {
            if (thrd_join(thread[thr], NULL) == thrd_error)
            {
                fprintf(stderr, "Could not join negamax worker thread #%d to the main thread.\n", thr);
            }
            
            TransTable_destroy(&thrTT[thr]);
        }
    }
    
    // Look for immediate wins
    for (thr = 0; thr < dropCount; thr++)
    {
        if (winOnFirst[thr])
        {
            colN = dropList[thr] & 0xf;
            
            switch (dropList[thr] >> 4)
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

void Negamax_results(Make7* restrict _m7, Result *_r1, Result *_r2, Result *_r3, Result *_best)
{    
    // Check to see if the mirror image of the game state is the same
    // If so, only search the left side of the grid
    bool mirror = Make7_symmetrical(_m7);
    uint8_t cEnd = mirror ? 4 : MAKE7_SIZE, tile, col;
    Make7 resultM7 = *_m7;
    
    // Flush results with unknown values
    for (tile = 0; tile < cEnd; tile++)
    {
        _r1[tile].wdl = _r2[tile].wdl = _r3[tile].wdl = UNKNOWN_CHAR;
    }
    
    // Loop through all possible drop moves and assign results
    for (tile = 1; tile <= 3; tile++)
    {
        printf("%d ", tile);
        
        for (col = 0; col < cEnd; col++)
        {
#ifdef __unix__
            fflush(stdout);
#endif
            if (Make7_drop(&resultM7, tile, col))
            {
                
                // Look for immediate wins
                if (Make7_tilesSumTo7(&resultM7))
                {
                    switch (tile)
                    {
                    case 1:
                        _r1[col].wdl = WIN_CHAR;
                        _r1[col].dt7 = 0;
                        
                        if (mirror)
                        {
                            _r1[MAKE7_SIZE_M1 - col].wdl = WIN_CHAR;
                            _r1[MAKE7_SIZE_M1 - col].dt7 = 0;
                        }
                        break;
                    case 2:
                        _r2[col].wdl = WIN_CHAR;
                        _r2[col].dt7 = 0;
                        
                        if (mirror)
                        {
                            _r2[MAKE7_SIZE_M1 - col].wdl = WIN_CHAR;
                            _r2[MAKE7_SIZE_M1 - col].dt7 = 0;
                        }
                        break;
                    case 3:
                        _r3[col].wdl = WIN_CHAR;
                        _r3[col].dt7 = 0;
                        
                        if (mirror)
                        {
                            _r3[MAKE7_SIZE_M1 - col].wdl = WIN_CHAR;
                            _r3[MAKE7_SIZE_M1 - col].dt7 = 0;
                        }
                    }
                }
                else
                {
                    TransTable_destroy(&table);
                    TransTable_initialize(&table, table.size);
                    
                    // Solve for each move, as there is no win
                    switch (tile)
                    {
                    case 1:
                        _r1[col] = Negamax_solve(&resultM7, &table, false);
                        Result_increment(&_r1[col]);
                        
                        if (mirror)
                        {
                            _r1[MAKE7_SIZE_M1 - col] = _r1[col];
                        }
                        break;
                    case 2:
                        _r2[col] = Negamax_solve(&resultM7, &table, false);
                        Result_increment(&_r2[col]);
                        
                        if (mirror)
                        {
                            _r2[MAKE7_SIZE_M1 - col] = _r2[col];
                        }
                        break;
                    case 3:
                        _r3[col] = Negamax_solve(&resultM7, &table, false);
                        Result_increment(&_r3[col]);
                        
                        if (mirror)
                        {
                            _r3[MAKE7_SIZE_M1 - col] = _r3[col];
                        }
                    }
                }
                
                resultM7 = *_m7;
            }
            
            // Print the results
            switch (tile)
            {
            case 1:
                Result_print(&_r1[col], _best);
                break;
            case 2:
                Result_print(&_r2[col], _best);
                break;
            case 3:
                Result_print(&_r3[col], _best);
            }
        }
        
        // Print it for the right half if the grid is symmetrical
        for (; mirror && (col < MAKE7_SIZE); col++)
        {
            switch (tile)
            {
            case 1:
                Result_print(&_r1[col], _best);
                break;
            case 2:
                Result_print(&_r2[col], _best);
                break;
            case 3:
                Result_print(&_r3[col], _best);
            }
        }
        puts("");
    }
}
