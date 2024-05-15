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
    _init->state = MCTS_UNSOLVED;
    // _init->depth = _ancest ? _ancest->depth + 1 : 0;
}

void MCTSNode_destroy(MCTSNode* restrict _dest)
{
    uint8_t node;
    
    if (_dest->descendant)
    {
        for (node = 0; node < _dest->count; node++)
        {
            MCTSNode_destroy(&_dest->descendant[node]);
        }
        
        free(_dest->descendant);
    }
}

void MCTSNode_print(const MCTSNode* restrict _PRINT)
{
    printf("\e[1m%d%c\e[0m: %lld %lld %d %.8f\n", _PRINT->move >> 4, 'A' + (_PRINT->move & 0xf), _PRINT->points, _PRINT->visits, _PRINT->count, MCTS_uct(_PRINT));
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
        MCTSNode_print(_PRINTALL);
    }
}

void MCTSNode_avgPoints(const MCTSNode* restrict _PTS)
{
    double avg;
    uint8_t tile, col, node;
    
    for (node = 0; _PTS && (node < _PTS->count); node++)
    {
        avg = (double)(_PTS->descendant[node].points) / _PTS->descendant[node].visits;
        tile = _PTS->descendant[node].move >> 4;
        col = _PTS->descendant[node].move & 0xf;
        printf("%d%c %.2f %lld %lld ", tile, col + 'A', avg, _PTS->descendant[node].points, _PTS->descendant[node].visits);
        NodeStatus_print(_PTS->descendant[node].state, true);
        puts("");
    }
}

double MCTS_uct(const MCTSNode* restrict _UCT)
{
    return ((double)(_UCT->points) / _UCT->visits) + sqrt(2.0 * log((double)(_UCT->ancestor->visits) / _UCT->visits));
}


void MCTS_updateState(MCTSNode* restrict _status)
{
    uint8_t draws, wins, unsolved, i;
    
    // Sum the game-theoretic state of all nodes, propagating it up the tree when we have a conclusive result
    for (draws = wins = unsolved = i = 0; i < _status->count; i++)
    {
        switch (_status->descendant[i].state)
        {
        case MCTS_LOSS: // One loss is sufficient to prove a win; stop the search
            _status->state = MCTS_WIN;
            return;
        case MCTS_DRAW:
            draws++;
            break;
        case MCTS_WIN:
            wins++;
            break;
        default:
            unsolved++;
            break;
        } 
    }
    
    if (!unsolved) // Draws and wins require solutions from all descendants
    {
        if (draws)
        {
            _status->state = MCTS_DRAW; // We draw if the opponent can draw but not win
        }
        else if (i && i == wins)
        {
            _status->state = MCTS_LOSS; // Opponent can win regardless of our moves
        }
    }
}

/*!
 *  @param _selector The MCTS node to select.
 *  @param _m7       The Make 7 game state to update.
 *  @return          The selected MCTS node.
 */
MCTSNode *MCTS_select(MCTSNode* restrict _selector, Make7* restrict _m7)
{
    double bestUCT, currUCT;
    MCTSNode *selected = NULL;
    uint8_t node;
    
    // As long as this node has descendants
    while (_selector->descendant)
    {
        for (bestUCT = -INFINITY, node = 0; node < _selector->count; node++)
        {
            // Check unvisited descendant nodes; select it for expansion (same as having a UCT value of INFINITY)
            if (!_selector->descendant[node].visits)
            {
                return _selector;
            }
            
            // Select the descendant with the highest UCT value, not exploring moves that lead to solved states
            if ((_selector->descendant[node].state == MCTS_UNSOLVED) && (currUCT = MCTS_uct(&_selector->descendant[node])) > bestUCT)
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
    unsigned randMove;
    uint8_t simulMoves[MAKE7_SIZE_X3], simulCount;
    
    // Play until the game is over
    for (;;)
    {
        // Get the list of possible drops to simulate
        Make7_generate(_simulMS, simulMoves, &simulCount);
        
        // If there are moves, select one at random and play that move; otherwise, return no reward to indicate a draw
        if (simulCount)
        {
            randMove = genrand_int32() % simulCount;
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
        MCTS_updateState(_backpropagator);        
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
    MCTSNode *bestNode = NULL;
    // long double currPoints, bestPoints = -INFINITY;
    unsigned long long bestVisits;
    uint8_t node, bestState, descLosses, descDraws, descUnsolved;
    
    // Count the number of unsolved, lost, and drawn descendants
    for (node = descLosses = descDraws = descUnsolved = 0; node < _root->count && !descLosses; node++)
    {
        switch (_root->descendant[node].state)
        {
        case MCTS_DRAW:
            descDraws++;
            break;
        case MCTS_LOSS:
            descLosses++;
            break;
        case MCTS_UNSOLVED:
            descUnsolved++;
            break;
        }
    }
    
    // Pick the descendant that's likely to lose, as this means the ancestor is favored to win
    // In other cases, delay the decision until all descendants have been decided
    
    if (descLosses)
    {
        bestState = MCTS_LOSS;
    }
    else if (descUnsolved)
    {
        bestState = MCTS_UNSOLVED;
    }
    else
    {
        if (descDraws)
        {
            bestState = MCTS_DRAW;
        }
        else
        {
            bestState = MCTS_WIN;
        }
    }
    
    // bestPoints = -INFINITY;
    // bestVisits = 0;
    
    // Choose the node with the highest mean points per visit
    /*for (node = 0; node < _root->count; node++)
    {
        if (_root->descendant[node].state == bestState && (currPoints = ((long double)(_root->descendant[node].points) / _root->descendant[node].visits)) > bestPoints)
        {
            bestPoints = currPoints;
            bestNode = &_root->descendant[node];
        }
    }*/
    
    // Choose the node with the highest visit count
    for (node = bestVisits = 0; node < _root->count; node++)
    {
        if ((_root->descendant[node].state == bestState) && (_root->descendant[node].visits > bestVisits))
        {
            bestVisits = _root->descendant[node].visits;
            bestNode = &_root->descendant[node];
        }
    }
    
    if (!bestNode)
    {
        bestNode = &_root->descendant[0];
    }
    
    return (MCTSResult) {(double)(bestNode->points) / bestNode->visits, bestNode->move, bestNode->state};
}

/*
int SimulThread_simulate(void *_args)
{
    SimulThread *data = _args;
    
    // Check if it's time to stop
    while (atomic_load(&runMCTS))
    {
        // Wait for the start signal
        Barrier_wait(data->start);
        
        // Simulate
        *(data->localRes) = MCTS_simulate(&data->localM7, data->localM7.plyNum & 1);
        
        // Signal that the simulation is done
        Barrier_wait(data->finish);
    }
    
    return 0;
}
*/

int ProgressThread_print(void *_args)
{
    ProgressThread *progress = _args;
    
    for (;;)
    {
        thrd_sleep(&(struct timespec){.tv_sec = 1}, NULL);
        
        if (atomic_load(&runMCTS) && progress->root->state == MCTS_UNSOLVED)
        {
            *progress->result = MCTS_best(progress->root);
            MCTS_progress(progress->result, progress->winConHandle, *(progress->iters), ++(*progress->seconds), progress->root->state);
        }
        else
        {
            break;
        }
    }   
    
    return 0;
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
    
    double oneTile[MAKE7_SIZE], twoTile[MAKE7_SIZE], threeTile[MAKE7_SIZE];
    long long i, secs, sims;
    uint8_t tile, col, state1[MAKE7_SIZE], state2[MAKE7_SIZE], state3[MAKE7_SIZE];
    ProgressThread progThread;
    thrd_t ioThread;
    
    Make7 mctsM7 = *_M7;
    
    /*thrd_t *simulators;
    Barrier simulStart, simulEnd;
    SimulThread *simulArgs;
    long numThreads;
    int tid;

#if defined(_WIN64) || defined(_WIN32)
    numThreads = GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS) - 1;
#elifdef __unix__
    numThreads = sysconf(_SC_NPROCESSORS_ONLN) - 1; // -1 to exclude the search thread
#else
    numThreads = 1;
#endif
    
    //numThreads = 1;
    
    signed long long threadSims[numThreads];    
    
    // Create simulation threads based on system hardware
    if (!(simulators = malloc(sizeof(*simulators) * numThreads)))
    {
        printf("Could not allocate memory for simulation threads.\n");
        exit(EXIT_FAILURE);
    }
    
    if (!(simulArgs = malloc(sizeof(*simulArgs) * numThreads)))
    {
        printf("Could not allocate memory to store simulation thread arguments.\n");
        exit(EXIT_FAILURE);
    }
    
    // To take into account the search thread
    Barrier_initialize(&simulStart, numThreads + 1);
    Barrier_initialize(&simulEnd, numThreads + 1);
    
    for (i = 0; i < numThreads; i++)
    {
        simulArgs[i] = (SimulThread) {.localRes = &threadSims[i],
                                      .start = &simulStart,
                                      .finish = &simulEnd,
                                      .seed = time(NULL) + clock() + i,
                                      .id = i};
                                      
        printf("%x\n", &simulArgs[i].localM7);
        
        switch (thrd_create(&simulators[i], SimulThread_simulate, &simulArgs[i]))
        {
        case thrd_nomem:
            printf("Ran out of memory creating simulation thread #%lld.\n", i);
            exit(EXIT_FAILURE);
        case thrd_error:
            printf("Could not create simulation thread #%lld.\n", i);
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }*/
    
    progThread = (ProgressThread) {.root = &root,
                                   .result = &result,
                                   .iters = &i,
                                   .seconds = &secs};

#if defined(_WIN64) || defined(_WIN32)
    progThread.winConHandle = winTerm;
#else
    progThread.winConHandle = NULL;
#endif
    
    switch (thrd_create(&ioThread, ProgressThread_print, &progThread))
    {
    case thrd_nomem:
        printf("Ran out of memory creating simulation thread #%lld.\n", i);
        exit(EXIT_FAILURE);
    case thrd_error:
        printf("Could not create simulation thread #%lld.\n", i);
        exit(EXIT_FAILURE);
    default:
        break;
    }
    
    // Catch SIGINT to stop the search
    signal(SIGINT, MCTS_stop);
    MCTSNode_initialize(&root, NULL, 0);
    
    for (i = secs = 0; atomic_load(&runMCTS) && root.state == MCTS_UNSOLVED; i++)
    {
        // Selection
        leaf = MCTS_select(&root, &mctsM7);
        
        // Expansion
        if (!MCTS_expand(leaf, &mctsM7))
        {
            // On no more memory, restore the game state and stop; won't happen if virtual memory is unlimited
            mctsM7 = *_M7;
            break;
        }
        
        // Pick a random move to simulate and update the game state
        if (leaf->count)
        {
            leaf = &leaf->descendant[genrand_int32() % leaf->count];
            Make7_drop(&mctsM7, leaf->move >> 4, leaf->move & 0xf);
            sims = MCTS_simulate(&mctsM7, mctsM7.plyNum & 1);
            
            /*// Copy the game state to the threads
            for (tid = 0; tid < numThreads; tid++)
            {
                simulArgs[tid].localM7 = mctsM7;
            }
            
            // Start them
            Barrier_wait(&simulStart);
            
            // Wait for all to finish
            Barrier_wait(&simulEnd);
            
            // Add simulation results
            for (tid = 0; tid < numThreads; tid++)
            {
                sims += threadSims[tid];
            }*/
        }
        else
        {
            sims = 0;
            
            // Check immediate wins and losses, rewarding them with higher scores
            if (Make7_tilesSumTo7(&mctsM7))
            {
                sims = MAKE7_AREA_P1 - mctsM7.plyNum;
                leaf->state = MCTS_LOSS;
            }
            else if (Make7_noMoreMoves(&mctsM7))
            {
                leaf->state = MCTS_DRAW;
            }
        }
          
        //sims = Make7_tilesSumTo7(&mctsM7) ? MAKE7_AREA_P1 - leaf->depth : MCTS_simulate(&mctsM7, leaf->depth & 1);
        
        // Simulation and backpropagation
        MCTS_backpropagate(leaf, sims);
        
        // Reset the game state for the next loop
        mctsM7 = *_M7;
        
        // Compute best move found and print the progress
        /*if ((elapsed = difftime(time(NULL), progress)) >= 1)
        {
            result = MCTS_best(&root);
            
#if defined(_WIN64) || defined(_WIN32)
            MCTS_progress(&result, winTerm, i, ++secs, root.state);
#else
            MCTS_progress(&result, NULL, i, ++secs, root.state);
#endif
            
            progress = time(NULL);
        }*/
    }
    
    // Get the best move by average points per visit
    result = MCTS_best(&root);
    printf("\a ");
    
    if (_OUTPUT)
    {
        if (root.state != MCTS_UNSOLVED)
        {
#ifdef _WIN64
            MCTS_progress(&result, winTerm, i, !secs ? 1 : secs, root.state);
#else
            MCTS_progress(&result, NULL, i, !secs ? 1 : secs, root.state);
#endif
        }
        
        //root.state != MCTS_UNSOLVED ? NodeStatus_print(root.state, false) : puts("");
        puts("");
        
        // Initialize the array of points for invalid moves
        for (i = 0; i < MAKE7_SIZE; i++)
        {
            oneTile[i] = twoTile[i] = threeTile[i] = MCTS_INVALID;
            state1[i] = state2[i] = state3[i] = MCTS_UNSOLVED;
        }
        
        // Copy the points to the array
        for (i = 0; i < root.count; i++)
        {
            tile = root.descendant[i].move >> 4;
            col = root.descendant[i].move & 0xf;
            
            switch (tile)
            {
            case 1:
                oneTile[col] = (double)(root.descendant[i].points) / root.descendant[i].visits;
                state1[col] = root.descendant[i].state;
                break;
            case 2:
                twoTile[col] = (double)(root.descendant[i].points) / root.descendant[i].visits;
                state2[col] = root.descendant[i].state;
                break;
            case 3:
                threeTile[col] = (double)(root.descendant[i].points) / root.descendant[i].visits;
                state3[col] = root.descendant[i].state;
                break;
            }
        }
        
        // Print the point statistics by tile and column
#if defined(_WIN64) || defined(_WIN32)
        MCTS_pointStats(winTerm, oneTile, twoTile, threeTile, state1, state2, state3);
#else
        MCTS_pointStats(NULL, oneTile, twoTile, threeTile, state1, state2, state3);
#endif
        // MCTSNode_avgPoints(&root);
    }
    
    /*for (i = 0; i < numThreads; i++)
    {
        if (thrd_join(simulators[i], NULL) == thrd_error)
        {
            printf("Could not join simulation thread #%lld\n", i);
            exit(EXIT_FAILURE);
        }
    }*/
    
    if (thrd_join(ioThread, NULL) == thrd_error)
    {
        puts("Could not join the I/O thread");
        exit(EXIT_FAILURE);
    }
    
    signal(SIGINT, SIG_DFL);
    MCTSNode_destroy(&root);
    /*Barrier_destroy(&simulStart);
    Barrier_destroy(&simulEnd);
    free(simulators);
    free(simulArgs);*/
    atomic_store(&runMCTS, true);
    
    return result.bestMove;
}

void MCTS_progress(const MCTSResult* restrict _RESULT, void* restrict _WIN_HND, const unsigned long long _ITERS, const unsigned long long _SECS, const uint8_t _STATE)
{
#if defined(_WIN64) || defined(_WIN32)
    HANDLE *winTerm = _WIN_HND;
#else
    (void)(_WIN_HND);
#endif
    
    printf("\r");
    
    if (_RESULT->meanPts <= -1.0f)
    {
        
#if defined(_WIN64) || defined(_WIN32)
        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
        printf("\e[1;31m");
#endif
        
    }
    else if ((_RESULT->meanPts > -1.0f) && (_RESULT->meanPts < 1.0f))
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
    
    switch (_STATE)
    {
    case MCTS_LOSS:
    
#if defined(_WIN64) || defined(_WIN32)
        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
        printf("\e[0m\e[1;31m");
#endif
    
        break;
    case MCTS_DRAW:
    
#if defined(_WIN64) || defined(_WIN32)
        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
        printf("\e[0m\e[1;33m");
#endif
    
        break;
    case MCTS_WIN:
    
#if defined(_WIN64) || defined(_WIN32)
        SetConsoleTextAttribute(*winTerm, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
        printf("\e[0m\e[1;32m");
#endif
    
    default:
        break;
    }

#if defined(_WIN64) || defined(_WIN32)
    printf("%d%c %.3f ", (_RESULT->bestMove >> 4), 'A' + (_RESULT->bestMove & 0xf), _RESULT->meanPts);
    SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    printf("%llu %llu %llu", _ITERS, (_ITERS / _SECS), _SECS);
#else
    // printf("%d%c %.3f\e[0m %llu %llu %llu", (_RESULT->bestMove >> 4), 'A' + (_RESULT->bestMove & 0xf), _RESULT->meanPts, _ITERS, (_ITERS / _SECS), _SECS);
    
    printf("%d%c ", (_RESULT->bestMove >> 4), 'A' + (_RESULT->bestMove & 0xf));
    _STATE != MCTS_UNSOLVED ? NodeStatus_print(_STATE, false) : printf("%.3f", _RESULT->meanPts);
    printf("\e[0m %llu %llu %llu", _ITERS, (_ITERS / _SECS), _SECS);
    
#endif
    
#ifdef __unix__
    fflush(stdout);
#endif
    
}

void MCTS_pointStats(void* restrict _WIN_HND, const double* restrict _1T_PTS, const double* restrict _2T_PTS, const double* restrict _3T_PTS, const uint8_t *_1T_STATES, const uint8_t *_2T_STATES, const uint8_t *_3T_STATES) 
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
                if (_1T_STATES[col] != MCTS_UNSOLVED)
                {
                    switch (_1T_STATES[col])
                    {
                    case MCTS_LOSS:
                        printf("\e[1;32m");
                        break;
                    case MCTS_DRAW:
                        printf("\e[1;33m");
                        break;
                    case MCTS_WIN:
                        printf("\e[1;31m");
                    default:
                        break;
                    }
                    
                    NodeStatus_print(_1T_STATES[col], true);
                    printf("\e[0m ");
                }               
                else
                {
                    if ((_1T_PTS[col] != MCTS_INVALID) && (_1T_PTS[col] <= -1.0))
                    {
                        
#if defined(_WIN64) || defined(_WIN32)
                        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
                        printf("\e[1;31m");
#endif
                        
                    }
                    else if ((_1T_PTS[col] > -1.0) && (_1T_PTS[col] < 1.0))
                    {
                        
#if defined(_WIN64) || defined(_WIN32)
                        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                        printf("\e[1;33m");
#endif
                        
                    }
                    else if (_1T_PTS[col] >= 1.0)
                    {
                        
#if defined(_WIN64) || defined(_WIN32)
                        SetConsoleTextAttribute(*winTerm, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                        printf("\e[1;32m");
#endif
                        
                    }
                
#if defined(_WIN64) || defined(_WIN32)
                    (_1T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2f ", _1T_PTS[col]);
#else
                    (_1T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2f\e[0m ", _1T_PTS[col]);
#endif               
                }             
                break;
            case 2:
                if (_2T_STATES[col] != MCTS_UNSOLVED)
                {
                    switch (_2T_STATES[col])
                    {
                    case MCTS_LOSS:
                        printf("\e[1;32m");
                        break;
                    case MCTS_DRAW:
                        printf("\e[1;33m");
                        break;
                    case MCTS_WIN:
                        printf("\e[1;31m");
                    default:
                        break;
                    }
                    
                    NodeStatus_print(_2T_STATES[col], true);
                    printf("\e[0m ");
                }               
                else
                {
                    if ((_2T_PTS[col] != MCTS_INVALID) && (_2T_PTS[col] <= -1.0))
                    {
                        
#if defined(_WIN64) || defined(_WIN32)
                        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
                        printf("\e[1;31m");
#endif
                        
                    }
                    else if ((_2T_PTS[col] > -1.0) && (_2T_PTS[col] < 1.0))
                    {
                        
#if defined(_WIN64) || defined(_WIN32)
                        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                        printf("\e[1;33m");
#endif
                        
                    }
                    else if (_2T_PTS[col] >= 1.0)
                    {
                        
#if defined(_WIN64) || defined(_WIN32)
                        SetConsoleTextAttribute(*winTerm, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                        printf("\e[1;32m");
#endif
                        
                    }
                
#if defined(_WIN64) || defined(_WIN32)
                    (_2T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2f ", _2T_PTS[col]);
#else
                    (_2T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2f\e[0m ", _2T_PTS[col]);
#endif
                }
                break;
            case 3:
                if (_3T_STATES[col] != MCTS_UNSOLVED)
                {
                    switch (_3T_STATES[col])
                    {
                    case MCTS_LOSS:
                        printf("\e[1;32m");
                        break;
                    case MCTS_DRAW:
                        printf("\e[1;33m");
                        break;
                    case MCTS_WIN:
                        printf("\e[1;31m");
                    default:
                        break;
                    }
                    
                    NodeStatus_print(_3T_STATES[col], true);
                    printf("\e[0m ");
                }               
                else
                {
                    if ((_3T_PTS[col] != MCTS_INVALID) && (_3T_PTS[col] <= -1.0))
                    {
                        
#if defined(_WIN64) || defined(_WIN32)
                        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
                        printf("\e[1;31m");
#endif
                        
                    }
                    else if ((_3T_PTS[col] > -1.0) && (_3T_PTS[col] < 1.0))
                    {
                        
#if defined(_WIN64) || defined(_WIN32)
                        SetConsoleTextAttribute(*winTerm, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                        printf("\e[1;33m");
#endif
                        
                    }
                    else if (_3T_PTS[col] >= 1.0)
                    {
                        
#if defined(_WIN64) || defined(_WIN32)
                        SetConsoleTextAttribute(*winTerm, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
                        printf("\e[1;32m");
#endif
                        
                    }
                    
#if defined(_WIN64) || defined(_WIN32)
                    (_3T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2f ", _3T_PTS[col]);
#else
                    (_3T_PTS[col] == MCTS_INVALID) ? printf("-- ") : printf("%.2f\e[0m ", _3T_PTS[col]);
#endif
                }
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
    init_genrand(time(NULL) + clock() + mrt->id);
    
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
            leaf = &leaf->descendant[genrand_int32() % leaf->count];
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
    double gOneTile[MAKE7_SIZE], gTwoTile[MAKE7_SIZE], gThreeTile[MAKE7_SIZE];
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
    thrBestRes = (MCTSResult) {0.0, 0, 0};
    
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
        thrBestRes.meanPts = (double)(bestRoot->points) / bestRoot->visits;
        mtx_unlock(&rootLock);
        
#if defined(_WIN64) || defined(_WIN32)
        MCTS_progress(&thrBestRes, winTerm, atomic_load(&i), ++secs, 0);
#else
        MCTS_progress(&thrBestRes, NULL, atomic_load(&i), ++secs, 0);
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
                gOneTile[col] = (double)(thrGRoot[thr].points) / thrGRoot[thr].visits;
                break;
            case 2:
                gTwoTile[col] = (double)(thrGRoot[thr].points) / thrGRoot[thr].visits;
                break;
            case 3:
                gThreeTile[col] = (double)(thrGRoot[thr].points) / thrGRoot[thr].visits;
            }
        }
        
        puts("\a");
        
#if defined(_WIN64) || defined(_WIN32)
        MCTS_pointStats(winTerm, gOneTile, gTwoTile, gThreeTile, NULL, NULL, NULL);
#else
        MCTS_pointStats(NULL, gOneTile, gTwoTile, gThreeTile, NULL, NULL, NULL);
#endif
    }

    signal(SIGINT, SIG_DFL);
    atomic_store(&runMCTS, true);
    mtx_destroy(&rootLock);
    
    return thrBestRes.bestMove;
}

void NodeStatus_print(const NodeStatus _NS, const bool _FLIP_STATUS)
{
    if (_NS == MCTS_WIN)
    {
        _FLIP_STATUS ? printf("LOSS") : printf("WIN");
    }
    else if (_NS == MCTS_DRAW)
    {
        printf("DRAW");
    }
    else if (_NS == MCTS_LOSS)
    {
        _FLIP_STATUS ? printf("WIN") : printf("LOSS");
    }
    else
    {
        printf("UNSOLVED");
    }
}