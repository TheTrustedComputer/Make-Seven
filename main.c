#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <pthread.h> // MinGW does not have the C11 threads.h header
#elif defined(__unix__)
#include <unistd.h>
#include <threads.h>
#endif

#ifdef __linux__
#include <sys/sysinfo.h>
#include <sys/resource.h>
#endif

#include "makeseven.c"
#include "transpositiontable.c"
#include "alphabeta.c"

int main(int argc, char **argv) {
	/* Variable declarations */
#ifdef _WIN32
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
	// The Make Seven game and solution results
	MakeSeven ms;
	Result r, r1[7], r2[7], r3[7];
	
	// Character input from user
	char input;
	
	// Statistics on solving time, number of positions evaluated, and best move
	time_t stopwatch;
	double sec, npsec;
	short best;
	
	// Flag to stop running the solver
	int stopRunning;
	
	// Lower the process priority for other processes
#ifdef _WIN32
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
#elif defined(__linux__)
	setpriority(getpid(), PRIO_PROCESS, 10);
#endif
	
#ifdef FIXED_TT_SIZE // Tell the compiler to use a fixed size for the transposition table
	TranspositionTable_initialize(&table, TT_HASHSIZE);
#else
#ifdef __linux__ // Linux: get host memory size using sysinfo; set transposition table size to half of host memory
	{
		// Structure for system information
		struct sysinfo sysSpecs;
		unsigned long gigs, power2;
		
		if (sysinfo(&sysSpecs) != -1) { // Request system information
			// Fetch half of host memory size in gigabytes
			gigs = sysSpecs.totalram / (1024 * 1024 * 1024) / 2;
			// Round up to exactly a power of two
			for (power2 = 1; power2 < gigs; power2 <<= 1);
			// Keep asking for memory but use half of that memory if unable to get it reserved
			while (!TranspositionTable_initialize(&table, TT_HASHSIZE * power2)) {
				power2 >>= 1;
			}
		}
		else { // Use a gigabyte if cannot get system information
			TranspositionTable_initialize(&table, TT_HASHSIZE);
		}
	}
#elif defined(_WIN32) // Windows: get host memory size using GlobalMemoryStatusEx; this works with virtual machines unlike GetPhysicallyInstalledSystemMemory
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
		
		if (success0) { // If either succeeded, initialize transposition table with half of host memory
			for (upower2 >>= 1; !TranspositionTable_initialize(&table, TT_HASHSIZE * upower2); upower2 >>= 1);
		}
		else if (success1) {
			for (power2 >>= 1; !TranspositionTable_initialize(&table, TT_HASHSIZE * power2); power2 >>= 1);
		}
		else { // When neither of the above worked, use a gigabyte
			TranspositionTable_initialize(&table, TT_HASHSIZE);
		}
	}
#else
	// Default to one gigabyte on all other platforms
	TranspositionTable_initialize(&table, TT_HASHSIZE);
#endif
#endif
	
	// Prepare game and alpha-beta move ordering array
	AlphaBeta_getColumnMoveOrder();
    MakeSeven_initialize(&ms);

	// Print greetings and details
	printf("Make Seven solver by TheTrustedComputer\n");
	printf("Hash table of %u entries\n", table.size);;
	
	// Main solving loop
	for (stopRunning = 0; !stopRunning;) {
		if (argc >= 2) { // Get move sequence from command line arguments
			MakeSeven_sequence(&ms, argv[1]);
			stopRunning = 1;
			if (MakeSeven_tilesSumToSeven(&ms)) {
				MakeSeven_print(&ms);
				printf("\e[1m%s wins!\e[0m\n", ms.plyNumber & 1u ? MAKESEVEN_PLAYER1_NAME : MAKESEVEN_PLAYER2_NAME);
				break;
			}
		}
		else {
			while ((input = getchar()) != EOF) { // Get sequence from user input
				if (MakeSeven_tilesSumToSeven(&ms)) {
					MakeSeven_print(&ms);
					printf("\e[1mGame over. %s made seven!\e[0m\n", ms.plyNumber & 1u ? MAKESEVEN_PLAYER1_NAME : MAKESEVEN_PLAYER2_NAME);
					MakeSeven_initialize(&ms);
					continue;
				}
				if (MakeSeven_hasNoMoreMoves(&ms)) {
					MakeSeven_print(&ms);
					printf("\e[1m%s has no more number tiles remaining. Draw!\e[0m\n", ms.plyNumber & 1u ? MAKESEVEN_PLAYER2_NAME : MAKESEVEN_PLAYER1_NAME);
					MakeSeven_initialize(&ms);
					continue;
				}
				if (!MakeSeven_getUserCharInput(&ms, input)) {
					if (input == '\n') {
						break;
					}
				}
			}
		}
		
		// Print game state
		MakeSeven_print(&ms);
		
		// Time the search
		nodes = 0ull;
		stopwatch = clock();
		r = AlphaBeta_solve(&ms, true);
		stopwatch = clock() - stopwatch;

		sec = (double)(stopwatch) / CLOCKS_PER_SEC;
		npsec = (double)(nodes) / (sec ? sec : sec + 1.0);
		printf("\a");
#ifdef _WIN32
		if (r.wdl == DRAW_CHAR) {
			SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			printf("%s ", DRAW_TEXT);
			SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}
		else {
			Result_print(&r, &r);
		}
#else
		(r.wdl == DRAW_CHAR) ? printf("\e[1;33m%s\e[0m ", DRAW_TEXT) : Result_print(&r, &r);
#endif
		printf("%llu %.0f %.3f\n", nodes, npsec, sec);
		if (argc < 2) {
			AlphaBeta_getMoveScores(&ms, r1, r2, r3, &r);
			best = Result_getBestMove(r1, r2, r3);
			printf("\aBest: %d%c\n", (best >> 4), (best & 0b1111) + 'A');
			MakeSeven_initialize(&ms);
			TranspositionTable_reset(&table);
		}
	}
	TranspositionTable_destroy(&table);
    return 0;
}
