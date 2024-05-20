/*
    Copyright (C) 2020- TheTrustedComputer
    
    The driver function of the Make 7 game solver.
    It reads command-line arguments, sets flags, inputs move sequences, and solves positions.
    This repeats indefinitely until the user terminates the program.
*/

#define _POSIX_C_SOURCE 200809 // clock_gettime()

#include <string.h>
#include <assert.h>
#include <errno.h>

#if defined(_WIN64) || defined(_WIN32)
#include <windows.h>
#endif

#ifdef __unix__
#include <unistd.h>
#endif

#ifdef __linux__
#include <sys/sysinfo.h>
#include <sys/resource.h>
#endif

// We will include source files directly for maximum performance
#include "mt19937ar-cok.h"
//#include "mt19937-64.h"
#include "make7.c"
#include "table.c"
#include "result.c"
#include "negamax.c"
//#include "barrier.c"
#include "mcts.c"

static inline void stopPGO(int UNUSED)
{
    (void)(UNUSED);
    exit(0);
}

int main(int argc, char **argv)
{
    /* Variable declarations */
#if defined(_WIN64) || defined(_WIN32)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
    
    // The Make 7 game and solution results per tile
    static Make7 ms;
    static Result r, r1[MAKE7_SIZE], r2[MAKE7_SIZE], r3[MAKE7_SIZE];
    
    // To see if the game state is the same after solving
    Make7 oldMS;
    
    // The best move found by Monte Carlo tree search; list of possible moves; total moves
    uint8_t best, mctsMove, totalMoves, movesList[MAKE7_SIZE_X3], m;
    
    // Character input; move input from user; interactive mode type
    char input, humanInput[3], type, argSeq[MAKE7_AREA_X2 + 1];
    
    // Statistics on solving time, and number of positions evaluated
    struct timespec parallelStart, parallelEnd;
    clock_t stopwatch;
    double sec, npsec;
    
    // Flags for the main loop
    bool over, running, notFixedTable, monteCarloTS, interactive, parallel, humanMove, invalidMove, pgo;
    
    // The final calculated table size, used when setting up the transposition table for the first time
    size_t finalTTSize;
    
    // Pointer to iterate the results to check for correctness
    int res;
    
    // Lower our priority for other processes
#if defined(_WIN64) || defined(_WIN32)
    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
#elifdef __linux__
    setpriority(PRIO_PROCESS, getpid(), 19);
#endif
    
    // Read command-line arguments and set flags accordingly
    {
        int opt;
        bool argTableNotDone, argMCTSNotDone, argInteractNotDone, argParallelNotDone, argSwapNotDone, argPGONotDone;
        
        // Default flag values
        monteCarloTS = false;
        interactive = false;
        parallel = false;
        pgo = false;
        g_swapColors = false;
        argTableNotDone = true;
        argMCTSNotDone = true;
        argInteractNotDone = true;
        argParallelNotDone = true;
        argSwapNotDone = true;
        argPGONotDone = true;
        notFixedTable = true;
        finalTTSize = 1;
        argSeq[0] = '\0';
        
        if (argc >= 2)
        {
            for (opt = 1; opt < argc; opt++)
            {
                // Tokenize the arguments: check for switches
                if (argv[opt][0] == '-')
                {
                    // Help
                    if (!(strcmp(argv[opt], "-h") && strcmp(argv[opt], "-?") && strcmp(argv[opt], "--help")))
                    {
                        Make7_helpMessage(argv[0]);
                        return 0;
                    }
                    
                    // Others
                    else
                    {
                        // Interactivity
                        if (argInteractNotDone && !(strcmp(argv[opt], "-i") && strcmp(argv[opt], "--interactive")))
                        {
                            interactive = true;
                            argInteractNotDone = notFixedTable = false;
                        }
                        
                        // Monte Carlo tree search
                        else if (argMCTSNotDone && !(strcmp(argv[opt], "-m") && strcmp(argv[opt], "--mcts")))
                        {
                            monteCarloTS = true;
                            argMCTSNotDone = false;
                        }
                        
                        // Parallelization
                        else if (argParallelNotDone && !(strcmp(argv[opt], "-p") && strcmp(argv[opt], "--parallel")))
                        {
                            parallel = true;
                            argParallelNotDone = false;
                        }
                        
                        // Tile color swapping
                        else if (argSwapNotDone && !(strcmp(argv[opt], "-s") && strcmp(argv[opt], "--swap-colors")))
                        {
                            g_swapColors = true;
                            argSwapNotDone = false;
                        }
                        
                        // Transposition table size
                        else if (argTableNotDone && !(strcmp(argv[opt], "-t") && strcmp(argv[opt], "--table-size")))
                        {
                            opt++;
                            finalTTSize = argv[opt] ? atoi(argv[opt]) : 1;
                            argTableNotDone = notFixedTable = false;
                        }
                        
                        // Profile guided optimization
                        else if (argPGONotDone && !(strcmp(argv[opt], "-g") && strcmp(argv[opt], "--guided")))
                        {
                            pgo = true;
                            argPGONotDone = false;
                        }
                        
                        // Unknown switch
                        else
                        {
                            fprintf(stderr, "Could not understand the switch \"%s\". Please type \"-h\" for help.\n", argv[opt]);
                            return 1;
                        }
                    }
                }
                
                // Move sequence
                else if ((argv[opt][0] >= '1' && argv[opt][0] <= '3') && (((argv[opt][1] >= 'A') && (argv[opt][1] <= 'G')) || ((argv[opt][1] >= 'a') && (argv[opt][1] <= 'g'))))
                {
                    snprintf(argSeq, sizeof(argSeq), "%s", argv[opt]);
                }
            }
        }
    }
    
    if (!monteCarloTS)
    {
        if (notFixedTable)
        {
#ifdef __linux__ // Linux: get host memory size using sysinfo; set transposition table size to the host memory
            
            // Structure for system information
            struct sysinfo sysSpecs;
            unsigned long gigs, power2;
            
            if (sysinfo(&sysSpecs) != -1) // Request system information
            {                
                // Fetch the host memory size in gigabytes
                gigs = sysSpecs.totalram / 1073741824;
                
                // Round up to the nearest power of two that is less than or equal to the system memory size
                for (power2 = 1; power2 < gigs; power2 <<= 1);
                
                // Keep asking for memory but use half of that memory if unable to get it reserved
                while (power2 && !TransTable_initialize(&table, (finalTTSize = TT_HASHSIZE * power2)))
                {
                    power2 >>= 1;
                }
                
                if (!table.entry)
                {
                    fprintf(stderr, "Could not initialize the transposition table size to the installed memory size.\n");
                    exit(EXIT_FAILURE);
                }
            }
            else // Use a fallback if cannot get system information3
            {
                TransTable_initialize(&table, (finalTTSize = TT_HASHSIZE));
            }
            
#elif defined(_WIN64) || defined(_WIN32) // Windows: get host memory size using GlobalMemoryStatusEx; this works with virtual machines unlike GetPhysicallyInstalledSystemMemory
            
            MEMORYSTATUSEX memory;
            unsigned long long kilos, upower2;
            long long gigs, power2;
            bool success0, success1;
            memory.dwLength = sizeof(memory);
            
            kilos = 0ull;
            
            // Returns in kilobytes, divide by 1048576 to get gigabytes
            success0 = GetPhysicallyInstalledSystemMemory(&kilos);
            kilos /= 1048576;
            
            // Round up to nearest power of two
            for (upower2 = 1; upower2 < kilos; upower2 <<= 1);
            
            gigs = 0l;
            
            // A non-zero return value means it's okay; zero if cannot fetch installed memory size
            success1 = GlobalMemoryStatusEx(&memory);
            
            // Divide by 1024^3 to get gigabytes as it is in bytes
            gigs = memory.ullTotalPhys / 1073741824;
            
            for (power2 = 1; power2 < gigs; power2 <<= 1);
            
            // If either succeeded, initialize transposition table with half of host memory
            if (success0)
            {
                for (upower2 >>= 1; upower2 && !TransTable_initialize(&table, (finalTTSize = TT_HASHSIZE * upower2)); upower2 >>= 1);
            }
            else if (success1)
            {
                for (power2 >>= 1; upower2 && !TransTable_initialize(&table, (finalTTSize = TT_HASHSIZE * power2)); power2 >>= 1);
            }
            else // When neither of the above worked, use a gigabyte
            {
                TransTable_initialize(&table, (finalTTSize = TT_HASHSIZE));
            }
#else
            // Default to one gigabyte on all other platforms
            TransTable_initialize(&table, (finalTTSize = TT_HASHSIZE));
#endif
        }
        else if (!interactive)
        {
            // Use a fixed size for the transposition table
            if (!TransTable_initialize(&table, finalTTSize * (TT_HASHSIZE >> 1)))
            {
                fprintf(stderr, "Could not allocate memory for the transposition table. Please try a different size.\n"); 
                return 1;
            }
        }
    }
    
    over = false;
    running = true;
    
    // For parallelized negamax, each thread will have its own transposition table, so there is no need to allocate a global one
    if (parallel)
    {
        TransTable_destroy(&table);
    }
    
    // Seed the Mersenne Twister PRNG
    init_genrand(time(NULL) + clock());
     
    // Prepare alpha-beta move ordering array
    Negamax_setColumnMoveOrder();
    
    // Initialize the game with the starting position
    Make7_initialize(&ms);
    
    // Print greetings and details
    puts("Make 7 solver by TheTrustedComputer");
    
    if (interactive)
    {
        type = '0';
        humanMove = true;
        
        puts("Running in interactive mode\n");
        
        // Interactive loop
        while (running)
        {
            puts("0: Human vs. Human\n1: Human vs. Computer\n2: Computer vs. Human\n3: Computer vs. Computer");
            puts("When playing against the computer, hit Control+C to make it play its turn.");
            
            while ((input = getchar()) != EOF)
            {
                type = input;
                
                switch (input)
                {
                case '0':
                case '1':
                    humanMove = true;
                    goto Label_startGame;
                case '2':
                case '3':
                    humanMove = false;
                    goto Label_startGame;
                case '\n':
                    continue;
                default:
                    fprintf(stderr, "Please select an option from the list.\n");
                    while (getchar() != '\n');
                    continue;
                }
            }
                
            Label_startGame:
            while (getchar() != '\n');
            
            while (!over)
            {
                Make7_print(&ms);
                
                if (Make7_tilesSumTo7(&ms))
                {
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    printf("%s wins! Play again?\n", ms.turn ? "Player 1" : "Player 2");
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    puts("Y: Yes\nN: No");
#else
                    printf("\e[1m%s wins! Play again?\n\e[0mY: Yes\nN: No\n", ms.turn ? "Player 1" : "Player 2");
#endif
                    over = true;
                    continue;
                }
                else if (Make7_noMoreMoves(&ms) || Make7_gridFull(&ms))
                {
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    puts("Draw! Play again?");
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    puts("Y: Yes\nN: No");
#else
                    puts("\e[1mDraw! Play again?\n\e[0mY: Yes\nN: No");
#endif
                    over = true;
                    continue;
                }
                
                if (humanMove)
                {
                    Make7_generate(&ms, movesList, &totalMoves);
                    
                    for (m = 0; m < totalMoves; m++)
                    {
                        printf("%d%c ", movesList[m] >> 4, (movesList[m] & 0x7) + 'A');
                    }
                    
                    puts("");
                    
                    for (invalidMove = true; invalidMove;)
                    {
#ifdef _MSC_VER
                        scanf_s("%2s", humanInput, sizeof(humanInput) / sizeof(humanInput[0]));
#else
                        scanf("%2s", humanInput);
#endif
                        if (!humanInput[1] || !Make7_sequence(&ms, humanInput))
                        {
                            fprintf(stderr, "Please enter a valid move from the list.\n");
                            while (getchar() != '\n');
                            continue;
                        }
                        else
                        {
                            invalidMove = false;
                            while (getchar() != '\n');
                        }
                    }
                }
                else
                {
#if defined(_WIN64) || defined(_WIN32)
                    mctsMove = MCTS_search(&ms, &handle, false);
#else
                    mctsMove = MCTS_search(&ms, NULL, false);
#endif
                    Make7_drop(&ms, mctsMove >> 4, mctsMove & 0x7);
                    puts("");
                }
                
                // Switch player depending on the type
                if ((type == '1') || (type == '2'))
                {
                    humanMove = !humanMove;
                }
            }
            
            switch ((input = getchar()))
            {
            case 'Y':
            case 'y':
                Make7_initialize(&ms);
                over = false;
                break;
            default:
                running = false;
                break;
            }
        }
        
        return 0;
    }
    else
    {
        monteCarloTS ? puts("Using Monte Carlo tree search") : printf("Transposition table of %lu entries\n", table.size);
        
#ifdef NO_SLIDERS
        puts("Not utilizing sliding windows");
#else
        puts("Utilizing sliding windows");
#endif
        
        // Solving loop
        while (running)
        {       
            over = false;
            best = 0;
            
            // Are we running to generate a PGO (profile guided optimization) profile?
            if (pgo)
            {
                // Disable parallelization, as it can slow down the search significantly
                running = parallel = false;
                signal(SIGINT, stopPGO);
            }
            else
            {
                // Get move sequence from command line arguments
                if (argSeq[0])
                {
                    running = false;
                    Make7_sequence(&ms, argSeq);
                }
                else
                {
                    // Get sequence from user input
                    while ((input = getchar()) != EOF)
                    {
                        if (!Make7_getUserInput(&ms, input))
                        {
                            if (input == '\n')
                            {
                                break;
                            }
                        }
                    }
                }
            }
            
            // Print game state
            Make7_print(&ms);
            oldMS = ms;
            
            if (Make7_plyNum(&ms))
            {
                // Check if the move sequence results in a win, draw, or loss
                if (Make7_tilesSumTo7(&ms))
                {
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    printf("Game over. %s made 7!\n", ms.turn ? MAKE7_P1_NAME : MAKE7_P2_NAME);
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                    printf("\e[1mGame over; %s made 7!\e[0m\n", ms.turn ? MAKE7_P1_NAME : MAKE7_P2_NAME);
#endif
                    over = true;
                }
                else if (Make7_gridFull(&ms))
                {
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    puts("The grid is full. Draw!");
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                    puts("\e[1mThe grid is full. Draw!\e[0m");
#endif
                    over = true;
                }
                else if (Make7_noMoreMoves(&ms))
                {
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    printf("%s has no more number tiles remaining. Draw!\n", ms.turn ? MAKE7_P2_NAME : MAKE7_P1_NAME);
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                    printf("\e[1m%s has no more number tiles remaining. Draw!\e[0m\n", ms.turn ? MAKE7_P2_NAME : MAKE7_P1_NAME);
#endif
                    over = true;
                }
            }
            
            // Check the game over flag and restart the game
            if (over)
            {
                Make7_initialize(&ms);
                continue;
            }
            
            if (monteCarloTS)
            {
#if defined(_WIN64) || defined(_WIN32)
                parallel ? MCTS_rootParallel(&ms, &handle, true) : MCTS_search(&ms, &handle, true);
#else
                parallel ? MCTS_rootParallel(&ms, NULL, true) : MCTS_search(&ms, NULL, true);
#endif
            }
            else
            {
                // Time the search and print the solution, ensuring that the solution is valid for the game
                atomic_store(&nodes, 0);
                
                if (parallel)
                {
                    clock_gettime(CLOCK_MONOTONIC, &parallelStart);
                    r = Negamax_solve_parallel(&ms, true, r1, r2, r3, NULL, &best);
                    clock_gettime(CLOCK_MONOTONIC, &parallelEnd);
                    sec = (double)((parallelEnd.tv_sec - parallelStart.tv_sec) + (parallelEnd.tv_nsec - parallelStart.tv_nsec) / 1000000000.0);
                }
                else
                {
                    stopwatch = clock();
                    r = Negamax_solve(&ms, &table, true);
                    stopwatch = clock() - stopwatch;
                    sec = (double)(stopwatch) / CLOCKS_PER_SEC;
                }
                
                npsec = (double)(atomic_load(&nodes)) / (sec ? sec : sec + 1.0);
                assert((r.wdl == WIN_CHAR && !(r.dt7 & 1)) || (r.wdl == DRAW_CHAR) || (r.wdl == LOSS_CHAR && (r.dt7 & 1)) || (r.wdl == UNKNOWN_CHAR));
                assert((oldMS.player[0] == ms.player[0]) && (oldMS.player[1] == ms.player[1]) && (oldMS.tiles23[0] == ms.tiles23[0]) && (oldMS.tiles23[1] == ms.tiles23[1]));
                assert((oldMS.turn == ms.turn) && (oldMS.remaining[0] == ms.remaining[0]) && (oldMS.remaining[1] == ms.remaining[1]) && (oldMS.remaining[2] == ms.remaining[2]));
                printf("\a");
                
#if defined(_WIN64) || defined(_WIN32)
                if (r.wdl == DRAW_CHAR)
                {
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                    printf("%s ", DRAW_TEXT);
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
                else
                {
                    Result_print(&r, &r);
                }
#else
                (r.wdl == DRAW_CHAR) ? printf("\e[1;33m%s\e[0m ", DRAW_TEXT) : Result_print(&r, &r);
#endif
                printf("%llu %.0f %.3f\n", atomic_load(&nodes), npsec, sec);
                
                // Do not show the solutions for all the moves if ran with arguments
                if (!argSeq[0])
                {
                    if (parallel)
                    {
                        printf("1 ");
                        
                        for (res = 0; res < MAKE7_SIZE; res++)
                        {
                            Result_print(&r1[res], &r);
                        }
                        
                        printf("\n2 ");
                        
                        for (res = 0; res < MAKE7_SIZE; res++)
                        {
                            Result_print(&r2[res], &r);
                        }
                        
                        printf("\n3 ");
                        
                        for (res = 0; res < MAKE7_SIZE; res++)
                        {
                            Result_print(&r3[res], &r);
                        }
                        
                        puts("");
                        
                    }
                    else
                    {
                        Negamax_results(&ms, r1, r2, r3, &r);
                        best = Result_getBestMove(r1, r2, r3);
                    }
                    
                    printf("\aBest: %d%c\n", (best >> 4), (best & 0b1111) + 'A');
                    
                    for (res = 0; res < MAKE7_SIZE; res++)
                    {
                        assert((r1[res].wdl == WIN_CHAR && !(r1[res].dt7 & 1)) || (r1[res].wdl == DRAW_CHAR) || (r1[res].wdl == LOSS_CHAR && (r1[res].dt7 & 1)) || (r1[res].wdl == UNKNOWN_CHAR));
                        assert((r2[res].wdl == WIN_CHAR && !(r2[res].dt7 & 1)) || (r2[res].wdl == DRAW_CHAR) || (r2[res].wdl == LOSS_CHAR && (r2[res].dt7 & 1)) || (r2[res].wdl == UNKNOWN_CHAR));
                        assert((r3[res].wdl == WIN_CHAR && !(r3[res].dt7 & 1)) || (r3[res].wdl == DRAW_CHAR) || (r3[res].wdl == LOSS_CHAR && (r3[res].dt7 & 1)) || (r3[res].wdl == UNKNOWN_CHAR));
                    }
                }
                
                // Reset game and transposition table for another search
                // Most optimizing compilers will make these following statments take constant time
                // Compiling with MSVC, on the other hand, will not, slowing it down linearly
                if (!parallel)
                {
                    TransTable_destroy(&table);
                    
                    if (running)
                    {
                        TransTable_initialize(&table, table.size += 2);
                    }
                }
            }
            
            Make7_initialize(&ms);
        }
    }
    
    return 0;
}
