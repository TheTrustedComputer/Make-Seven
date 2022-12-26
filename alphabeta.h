#ifndef ALPHABETA_H
#define ALPHABETA_H

enum ResultChar {
	UNKNOWN_CHAR = '?', WIN_CHAR = 'W', DRAW_CHAR = 'D', LOSS_CHAR = 'L'
};

#define WIN_TEXT "WIN"
#define LOSS_TEXT "LOSS"
#define DRAW_TEXT "DRAW"
#define NONE_TEXT "-- "

enum AlphaBetaScore {
	AB_UNKNOWN = -1, AB_DRAW, AB_WIN = 2
};

#define RESULT_DRAW (Result) { DRAW_CHAR, -1 }

static unsigned long long nodes;
static int moveOrder[7], tableScore;
static uint64_t hash;
static TranspositionTable table;

typedef struct {
	char wdl;
	uint8_t dt7;
} Result;

void AlphaBeta_getColumnMoveOrder(void);
bool AlphaBeta_checkForSeven(MakeSeven*);

int AlphaBeta_negamax(MakeSeven*, int, int ,int);
int AlphaBeta_negamax_withMoveScores(MakeSeven*, int, int, int, int);

Result AlphaBeta_solve(MakeSeven*, const bool);
void AlphaBeta_getMoveScores(MakeSeven*, Result*, Result*, Result*, Result*);

void Result_print(Result*, Result*);
void Result_increment(Result*);
Result Result_getBestResult(Result*, Result*, Result*);
uint8_t Result_getBestMove(Result*, Result*, Result*);

#endif
