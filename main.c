#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "makeseven.c"
#include "transpositiontable.c"
#include "alphabeta.c"

int main(int argc, char **argv) {
	// Variable declarations
	char input;
	time_t stopwatch;
	double sec, npsec;
	short best;
	int i;
 	MakeSeven ms;
	Result r, r1[7], r2[7], r3[7];

	TranspositionTable_initialize(&table, TT_HASHSIZE);
	AlphaBeta_getColumnMoveOrder();
    MakeSeven_initialize(&ms);
	printf("Pressman Toy Make 7 solver by TheTrustedComputer\n");
	printf("Hash table of %u entries\n", table.size);;
	for (i = 0; !i;) {
		if (argc >= 2) {
			MakeSeven_sequence(&ms, argv[1]);
			++i;
			if (MakeSeven_tilesSumToSeven(&ms)) {
				MakeSeven_print(&ms);
				printf("%s wins!\n", ms.plyNumber & 1u ? MAKESEVEN_PLAYER1_NAME : MAKESEVEN_PLAYER2_NAME);
				break;
			}
		}
		else {
			while ((input = getchar()) != EOF) {
				if (MakeSeven_tilesSumToSeven(&ms)) {
					printf("Game over. %s has made seven!\n", ms.plyNumber & 1u ? MAKESEVEN_PLAYER1_NAME : MAKESEVEN_PLAYER2_NAME);
					while (input != '\n') {
						input = getchar();
					}
					break;
				}
				if (MakeSeven_hasNoMoreMoves(&ms)) {
					printf("%s has no more number tiles remaining. Draw!\n", ms.plyNumber & 1u ? MAKESEVEN_PLAYER2_NAME : MAKESEVEN_PLAYER1_NAME);
					while (input != '\n') {
						input = getchar();
					}
					break;
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
		nodes = 0ull;

		// Start and stop solving time
		stopwatch = clock();
		r = AlphaBeta_solve(&ms, true);
		stopwatch = clock() - stopwatch;

		sec = (double)stopwatch / CLOCKS_PER_SEC;
		npsec = (double)nodes / (sec ? sec : sec + 1.0);
		printf("\a");
		(r.wdl == DRAW_CHAR) ? printf("\e[1;33m%s\e[0m ", DRAW_TEXT) : Result_print(&r, &r);
		printf("%llu %.0f %.3f\n", nodes, npsec, sec);
		if (argc < 2) {
			AlphaBeta_getMoveScores(&ms, r1, r2, r3, &r);
			best = Result_getBestMove(r1, r2, r3);
			printf("\aBest: %d\n", best);
			MakeSeven_initialize(&ms);
			TranspositionTable_reset(&table);
		}
	}
	TranspositionTable_destroy(&table);
    return 0;
}
