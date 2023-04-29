/*
    Copyright (C) 2020- TheTrustedComputer
    
    The driver function of the Make Seven solver.
    It reads command-line arguments, sets flags, inputs a move sequence, and solves the game.
    It repeats indefinitely until the user terminates the program.
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
#include "mt19937-64.h"
#include "make7.c"
#include "table.c"
#include "result.c"
#include "negamax.c"
#include "mcts.c"

int main(int argc, char **argv)
{
    /* Variable declarations */
#if defined(_WIN64) || defined(_WIN32)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
    
    // The Make 7 game and solution results per tile
    static MakeSeven ms;
    static Result r, r1[MAKESEVEN_SIZE], r2[MAKESEVEN_SIZE], r3[MAKESEVEN_SIZE];
    
    // To see if the game state is the same after solving
    MakeSeven oldMS;
    
    // The best move found by Monte Carlo tree search; list of possible moves; total moves
    uint8_t best, mctsMove, totalMoves, movesList[MAKESEVEN_SIZE_X3], m;
    
    // Character input; move input from user; interactive mode type
    char input, humanInput[3], type, argSeq[MAKESEVEN_AREA_X2];
    
    // Statistics on solving time, and number of positions evaluated
    struct timespec parallelStart, parallelEnd;
    time_t stopwatch;
    double sec, npsec;
    
    // Flags for the main loop
    bool over, running, notFixedTable, monteCarloTS, interactive, parallel, humanMove, invalidMove;
    
    // The final calculated table size, used when reallocating
    int finalTTSize = 1;
    
    // Lower the process priority for other processes
#if defined(_WIN64) || defined(_WIN32)
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
#elifdef __linux__
    setpriority(PRIO_PROCESS, getpid(), 10);
#endif
    
    // Read command-line arguments and set flags accordingly
    {
        // Default flags
        int option;
        bool argMCTSNotDone, argTableNotDone, argInteractNotDone, argParallelNotDone;
        
        monteCarloTS = over = interactive = parallel = false;
        running = argMCTSNotDone = argTableNotDone = argInteractNotDone = argParallelNotDone = notFixedTable = true;
        argSeq[0] = '\0';
        
        if (argc >= 2)
        {
            for (option = 1; option < argc; ++option)
            {
                // Tokenize the arguments: check for switches
                if (argv[option][0] == '-')
                {
                    // Help
                    if (!((strcmp(argv[option], "-h") && strcmp(argv[option], "-?") && strcmp(argv[option], "--help"))))
                    {
                        MakeSeven_helpMessage(argv[0]);
                        return 0;
                    }
                    
                    // Others
                    else
                    {
                        // Interactivity
                        if (argInteractNotDone && !((strcmp(argv[option], "-i") && strcmp(argv[option], "--interactive"))))
                        {
                            interactive = true;
                            argInteractNotDone = notFixedTable = false;
                        }
                        
                        // Monte Carlo tree search
                        else if (argMCTSNotDone && !((strcmp(argv[option], "-m") && strcmp(argv[option], "--mcts"))))
                        {
                            monteCarloTS = true;
                            argMCTSNotDone = false;
                        }
                        
                        // Parallelization
                        else if (argParallelNotDone && !((strcmp(argv[option], "-p") && strcmp(argv[option], "--parallel"))))
                        {
                            parallel = true;
                            argParallelNotDone = false;
                        }
                        
                        // Transposition table size
                        else if (argTableNotDone && !((strcmp(argv[option], "-t") && strcmp(argv[option], "--table"))))
                        {
                            if (argv[++option])
                            {
                                finalTTSize = atoi(argv[option]);
                            }
                            else
                            {
                                fprintf(stderr, "Please specify the size of the transposition table in gigabytes.\n");
                                return 1;
                            }
                            
                            argTableNotDone = notFixedTable = false;
                        }
                        
                        // Unknown switch
                        else
                        {
                            fprintf(stderr, "Could not understand the switch \"%s\". Please type \"-h\" for help.\n", argv[option]);
                            return 1;
                        }
                    }
                }
                
                // Move sequence
                else if ((argv[option][0] >= '1' && argv[option][0] <= '3') && (((argv[option][1] >= 'A') && (argv[option][1] <= 'G')) || ((argv[option][1] >= 'a') && (argv[option][1] <= 'g'))))
                {
#ifdef _MSC_VER
                    strncpy_s(argSeq, sizeof(argSeq) / sizeof(argSeq[0]), argv[option], sizeof(argSeq) / sizeof(argSeq[0]));
#else
                    strncpy(argSeq, argv[option], sizeof(argSeq) / sizeof(argSeq[0]));
#endif
                }
            }
        }
    }
    
    if (!monteCarloTS)
    {
        if (notFixedTable)
        {
#ifdef __linux__ // Linux: get host memory size using sysinfo; set transposition table size to half of host memory
            {
                // Structure for system information
                struct sysinfo sysSpecs;
                unsigned long gigs, power2;
                
                if (sysinfo(&sysSpecs) != -1) // Request system information
                {                
                    // Fetch half of host memory size in gigabytes
                    gigs = sysSpecs.totalram / 2147483648ul;
                    
                    // Round up to exactly a power of two
                    for (power2 = 1; power2 < gigs; power2 <<= 1);
                    
                    // Keep asking for memory but use half of that memory if unable to get it reserved
                    while (!TranspositionTable_initialize(&table, (finalTTSize = TT_HASHSIZE * power2)))
                    {
                        power2 >>= 1;
                    }
                }
                else // Use a fallback if cannot get system information3
                {
                    TranspositionTable_initialize(&table, (finalTTSize = TT_HASHSIZE));
                }
            }
#elif defined(_WIN64) || defined(_WIN32) // Windows: get host memory size using GlobalMemoryStatusEx; this works with virtual machines unlike GetPhysicallyInstalledSystemMemory
            {
                MEMORYSTATUSEX memory;
                unsigned long long kilos, upower2;
                long gigs, power2;
                bool success0, success1;
                memory.dwLength = sizeof(memory);
                
                kilos = 0ull;
                
                // Returns in kilobytes, divide by 1048576 to get gigabytes
                success0 = GetPhysicallyInstalledSystemMemory(&kilos);
                kilos /= 1048576;
                
                // Round up to nearest power of two
                for (upower2 = 1; upower2 < kilos; upower2 <<= 1);
                
                gigs = 0l;
                
                // A non-zero return value means it's okay; if cannot fetch installed memory size
                success1 = GlobalMemoryStatusEx(&memory);
                
                // Divide by 1024^3 to get gigabytes as it is in bytes
                
                gigs = memory.ullTotalPhys / 1073741824;
                
                for (power2 = 1; power2 < gigs; power2 <<= 1);
                
                // If either succeeded, initialize transposition table with half of host memory
                if (success0)
                {
                    for (upower2 >>= 1; !TranspositionTable_initialize(&table, (finalTTSize = TT_HASHSIZE * upower2)); upower  2 >>= 1);
                }
                else if (success1)
                {
                    for (power2 >>= 1; !TranspositionTable_initialize(&table, (finalTTSize = TT_HASHSIZE * power2)); power2 >>= 1);
                }
                else // When neither of the above worked, use a gigabyte
                {
                    TranspositionTable_initialize(&table, (finalTTSize = TT_HASHSIZE));
                }
            }
#else
            // Default to one gigabyte on all other platforms
            TranspositionTable_initialize(&table, (finalTTSize = TT_HASHSIZE));
#endif
        }
        else if (!interactive)
        {
            // Use a fixed size for the transposition table
            if (!TranspositionTable_initialize(&table, finalTTSize * (TT_HASHSIZE / 2)))
            {
                fprintf(stderr, "Could not allocate memory for the transposition table. Please try a different size.\n"); 
                return 1;
            }
        }
    }
    
    // For parallelized negamax, each thread will have its own transposition table, so there is no need to allocate one
    if (parallel)
    {
        TranspositionTable_destroy(&table);
    }
    
    // Seed the Mersenne Twister PRNG
    init_genrand64(time(NULL) + clock());
     
    // Prepare alpha-beta move ordering array
    Negamax_setColumnMoveOrder();
    
    // Initialize the game with the starting position
    MakeSeven_initialize(&ms);
    
    // Print greetings and details
    puts("Make Seven solver by TheTrustedComputer");
    
    if (interactive)
    {
        type = '0';
        humanMove = true;
        
        puts("Interactive mode\n");
        
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
                MakeSeven_print(&ms);
                
                if (MakeSeven_tilesSumToSeven(&ms))
                {
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    printf("%s wins! Play again?\n", ms.plyNum & 1 ? MAKESEVEN_PLAYER1_NAME : MAKESEVEN_PLAYER2_NAME);
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    puts("Y: Yes\nN: No");
#else
                    printf("\e[1m%s wins! Play again?\n\e[0mY: Yes\nN: No\n", ms.plyNum & 1 ? MAKESEVEN_PLAYER1_NAME : MAKESEVEN_PLAYER2_NAME);
#endif
                    over = true;
                    continue;
                }
                else if (MakeSeven_hasNoMoreMoves(&ms) || MakeSeven_gridFull(&ms))
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
                    MakeSeven_generate(&ms, movesList, &totalMoves);
                    
                    for (m = 0; m < totalMoves; ++m)
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
                        if (!humanInput[1] || !MakeSeven_sequence(&ms, humanInput))
                        {
                            fprintf(stderr, "Please enter a valid move.\n");
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
                    MakeSeven_drop(&ms, mctsMove >> 4, mctsMove & 0x7);
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
                MakeSeven_initialize(&ms);
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
        monteCarloTS ? puts("Using Monte Carlo tree search") : printf("Hash table of %u entries\n", table.size);
        
        // Solving loop
        while (running)
        {        
            // Get move sequence from command line arguments
            if (argSeq[0])
            {
                running = false;
                MakeSeven_sequence(&ms, argSeq);
            }
            else
            {
                // Get sequence from user input
                while ((input = getchar()) != EOF)
                {
                    if (!MakeSeven_getUserCharInput(&ms, input))
                    {
                        if (input == '\n') {
                            break;
                        }
                    }
                }
            }
            
            oldMS = ms;
            
            // Print game state
            MakeSeven_print(&ms);
            
            // Check if the move sequence results in a win, draw, or loss
            if (MakeSeven_tilesSumToSeven(&ms))
            {
#if defined(_WIN64) || defined(_WIN32)
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                printf("Game over. %s made 7!\n", ms.plyNum & 1 ? MAKESEVEN_PLAYER1_NAME : MAKESEVEN_PLAYER2_NAME);
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                printf("\e[1mGame over. %s made 7!\e[0m\n", ms.plyNum & 1 ? MAKESEVEN_PLAYER1_NAME : MAKESEVEN_PLAYER2_NAME);
#endif
                over = true;
            }
            else if (MakeSeven_gridFull(&ms))
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
            else if (MakeSeven_hasNoMoreMoves(&ms))
            {
#if defined(_WIN64) || defined(_WIN32)
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                printf("%s has no more number tiles remaining. Draw!\n", ms.plyNum & 1 ? MAKESEVEN_PLAYER2_NAME : MAKESEVEN_PLAYER1_NAME);
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                printf("\e[1m%s has no more number tiles remaining. Draw!\e[0m\n", ms.plyNum & 1 ? MAKESEVEN_PLAYER2_NAME : MAKESEVEN_PLAYER1_NAME);
#endif
                over = true;
            }
            
            // Check the game over flag and restart the game
            if (over)
            {
                MakeSeven_initialize(&ms);
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
                nodes = 0ull;
                
                if (parallel)
                {
                    clock_gettime(CLOCK_MONOTONIC, &parallelStart);
                    r = Negamax_solveInParallel(&ms, true, r1, r2, r3, NULL, &best);
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
                
                npsec = (double)(nodes) / (sec ? sec : sec + 1.0);
                printf("\a");
                assert((r.wdl == WIN_CHAR && !(r.dt7 & 1)) || (r.wdl == DRAW_CHAR) || (r.wdl == LOSS_CHAR && (r.dt7 & 1)) || (r.wdl == UNKNOWN_CHAR));
                assert((oldMS.player[0] == ms.player[0]) && (oldMS.player[1] == ms.player[1]) && (oldMS.tiles23[0] == ms.tiles23[0]) && (oldMS.tiles23[1] == ms.tiles23[1]));
                assert((oldMS.plyNum == ms.plyNum) && (oldMS.remaining[0] == ms.remaining[0]) && (oldMS.remaining[1] == ms.remaining[1]) && (oldMS.remaining[2] == ms.remaining[2]));
                
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
                printf("%llu %.0f %.3f\n", nodes, npsec, sec);
                
                // Do not show the solutions for all the moves if ran with arguments
                if (!argSeq[0])
                {
                    int res;
                    
                    if (parallel)
                    {
                        printf("1 ");
                        
                        for (res = 0; res < MAKESEVEN_SIZE; ++res)
                        {
                            Result_print(&r1[res], &r);
                        }
                        
                        printf("\n2 ");
                        
                        for (res = 0; res < MAKESEVEN_SIZE; ++res)
                        {
                            Result_print(&r2[res], &r);
                        }
                        
                        printf("\n3 ");
                        
                        for (res = 0; res < MAKESEVEN_SIZE; ++res)
                        {
                            Result_print(&r3[res], &r);
                        }
                        
                        puts("");
                        
                    }
                    else
                    {
                        Negamax_getMoveScores(&ms, r1, r2, r3, &r);
                        best = Result_getBestMove(r1, r2, r3);
                    }
                    
                    printf("\aBest: %d%c\n", (best >> 4), (best & 0b1111) + 'A');
                    
                    for (res = 0; res < MAKESEVEN_SIZE; ++res)
                    {
                        assert((r1[res].wdl == WIN_CHAR && !(r1[res].dt7 & 1)) || (r1[res].wdl == DRAW_CHAR) || (r1[res].wdl == LOSS_CHAR && (r1[res].dt7 & 1)) || (r1[res].wdl == UNKNOWN_CHAR));
                        assert((r2[res].wdl == WIN_CHAR && !(r2[res].dt7 & 1)) || (r2[res].wdl == DRAW_CHAR) || (r2[res].wdl == LOSS_CHAR && (r2[res].dt7 & 1)) || (r2[res].wdl == UNKNOWN_CHAR));
                        assert((r3[res].wdl == WIN_CHAR && !(r3[res].dt7 & 1)) || (r3[res].wdl == DRAW_CHAR) || (r3[res].wdl == LOSS_CHAR && (r3[res].dt7 & 1)) || (r3[res].wdl == UNKNOWN_CHAR));
                    }
                }
                
                // Reset game and transposition table for another search
                // Most optimizing compilers will make these following statments constant
                // Compiling with MSVC, on the other hand, will not, slowing it down linearly
                if (!parallel)
                {
                    TranspositionTable_destroy(&table);
                    
                    if (running)
                    { 
                        TranspositionTable_initialize(&table, finalTTSize);
                    }
                }
            }
            
            MakeSeven_initialize(&ms);
        }
    }
    
    return 0;
}
