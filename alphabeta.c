#include "alphabeta.h"

void AlphaBeta_getColumnMoveOrder(void) {
	for (int i = 0; i < 7; ++i) { // Center to outermost
		moveOrder[i] = 7 / 2 + (1 - 2 * (i & 1)) * (i + 1) / 2;
	}
}

bool AlphaBeta_checkForSeven(MakeSeven *ms) {
	int i, j;
	// Make all possible drops and check for sum of seven, then undo the drops
	for (i = 1; i <= 3; ++i) {
		for (j = 0; j < 7; ++j) {
			if (MakeSeven_drop(ms, i, j)) {
				if (MakeSeven_tilesSumToSeven(ms)) {
					MakeSeven_undrop(ms);
					return true;
				}
				MakeSeven_undrop(ms);
			}
		}
	}
	return false;
}

int AlphaBeta_negamax(MakeSeven *ms, int depth, int alpha, int beta) {
	int parentScore, childScore, i, j;
	++nodes;
	// See if the score is in the transposition table
	if (abs((tableScore = TranspositionTable_load(&table, (hash = MakeSeven_hashEncode(ms)), ms->twoAndThreeTiles[0], ms->twoAndThreeTiles[1]))) >= AB_WIN) {
		return tableScore;
	}
	// Check for a "Make 7"
	if (AlphaBeta_checkForSeven(ms)) {
		return AB_WIN; // Current player wins
	}
	// Check if the player cannot make any more moves or hiting maxiumum depth
	if (!depth || MakeSeven_hasNoMoreMoves(ms)) {
		return AB_DRAW; // Assume draw
	}
	parentScore = alpha;
	for (i = 4; --i;) {
		for (j = 0; j < 7; ++j) {
			if (MakeSeven_drop(ms, i, moveOrder[j])) {
				// Drop tiles and see if our score beats the current best score
				if ((childScore = -AlphaBeta_negamax(ms, depth - 1, -beta, -alpha)) > parentScore) {
					parentScore = childScore;
				}
				MakeSeven_undrop(ms);
				// Update best score if it's better than the current best, and store that score
				if (alpha < parentScore) {
					TranspositionTable_store(&table, (hash = MakeSeven_hashEncode(ms)), ms->twoAndThreeTiles[0], ms->twoAndThreeTiles[1], (alpha = parentScore));
				}
				// Alpha cut-off
				if (alpha >= beta) {
					return alpha;
				}
			}
		}
	}
	// Store current score in the transposition table for later lookup
	TranspositionTable_store(&table, (hash = MakeSeven_hashEncode(ms)), ms->twoAndThreeTiles[0], ms->twoAndThreeTiles[1], alpha);
	return alpha;
}

int AlphaBeta_negamax_withMoveScores(MakeSeven *ms, int currDepth, int maxDepth, int alpha, int beta) {
	int parentScore, childScore, i, j;
	++nodes;
	if ((tableScore = TranspositionTable_load(&table,(hash = MakeSeven_hashEncode(ms)), ms->twoAndThreeTiles[0], ms->twoAndThreeTiles[1]))) {
		switch (TranspositionTable_loadBounds(&table, hash, ms->twoAndThreeTiles[0], ms->twoAndThreeTiles[1])) {
		case TT_LOWERBOUND:
			if (alpha < tableScore) {
				alpha = tableScore;
			}
			if (alpha >= beta) {
				return alpha;
			}
			break;
		case TT_UPPERBOUND:
			if (beta > tableScore) {
				beta = tableScore;
			}
			if (alpha >= beta) {
				return beta;
			}
		}
	}
	if (AlphaBeta_checkForSeven(ms)) {
		return AB_WIN;
	}
	if ((currDepth >= maxDepth) || MakeSeven_hasNoMoreMoves(ms)) {
		return AB_DRAW;
	}
	parentScore = alpha;
	for (i = 4; --i;) {
		for (j = 0; j < 7; ++j) {
			if (MakeSeven_drop(ms, i, moveOrder[j])) {
				if ((childScore = -AlphaBeta_negamax_withMoveScores(ms, currDepth + 1, maxDepth, -beta, -alpha)) > parentScore) {
					MakeSeven_variation[currDepth] = (i << 4) | j;
					parentScore = childScore;
				}
				MakeSeven_undrop(ms);
				if (alpha < parentScore) {
					TranspositionTable_storeBounds(&table, (hash = MakeSeven_hashEncode(ms)), ms->twoAndThreeTiles[0], ms->twoAndThreeTiles[1], (alpha = parentScore), TT_LOWERBOUND);
				}
				if (alpha >= beta) {
					return alpha;
				}
			}
		}
	}
	TranspositionTable_storeBounds(&table, (hash = MakeSeven_hashEncode(ms)), ms->twoAndThreeTiles[0], ms->twoAndThreeTiles[1], alpha, TT_UPPERBOUND);
	return alpha;
}

void AlphaBeta_getMoveScores(MakeSeven *ms, Result *r1, Result *r2, Result *r3, Result *best) {
	int i, j;
	if (r1 && r2 && r3) {
		for (i = 1; i <= 3; ++i) {
			printf("%d ", i);
			for (j = 0; j < 7; ++j) {
#ifdef __unix__
				fflush(stdout);
#endif
				r1[j].wdl = r2[j].wdl = r3[j].wdl = UNKNOWN_CHAR;
				if (MakeSeven_drop(ms, i, j)) {
					if (MakeSeven_tilesSumToSeven(ms)) {
						switch (i) {
						case 1:
							r1[j].wdl = WIN_CHAR;
							r1[j].dt7 = 0;
							break;
						case 2:
							r2[j].wdl = WIN_CHAR;
							r2[j].dt7 = 0;
							break;
						case 3:
							r3[j].wdl = WIN_CHAR;
							r3[j].dt7 = 0;
						}
					}
					else {
#ifdef __GNUC__
						TranspositionTable_destroy(&table);
						TranspositionTable_initialize(&table, TT_HASHSIZE);
#else
						TranspositionTable_reset(&table);
#endif
						switch (i) {
						case 1:
							r1[j] = AlphaBeta_solve(ms, false);
							Result_increment(&r1[j]);
							break;
						case 2:
							r2[j] = AlphaBeta_solve(ms, false);
							Result_increment(&r2[j]);
							break;
						case 3:
							r3[j] = AlphaBeta_solve(ms, false);
							Result_increment(&r3[j]);
						}
					}
					MakeSeven_undrop(ms);
				}
				switch (i) {
				case 1:
					Result_print(&r1[j], best);
					break;
				case 2:
					Result_print(&r2[j], best);
					break;
				case 3:
					Result_print(&r3[j], best);
				}
			}
			puts("");
		}
	}
}

Result AlphaBeta_solve(MakeSeven *ms, const bool VERBOSE) {
	int depth, maxDepth = 49 - ms->plyNumber, solution = AB_DRAW;
	for (depth = 0; depth < maxDepth; ++depth) {
		if (VERBOSE) {
			printf("\rSolving...%d\r", depth);
			fflush(stdout);
		}
		if (abs((solution = AlphaBeta_negamax(ms, depth, -AB_WIN, AB_WIN))) >= AB_WIN) {
			return (Result) { solution > 0 ? WIN_CHAR : LOSS_CHAR, depth };
		}
	}
	return RESULT_DRAW;
}

void Result_print(Result *current, Result *best) {
#ifdef _WIN32 
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
	switch (current->wdl) {
	default:
	case UNKNOWN_CHAR:
	case '\0':
		printf(NONE_TEXT);
		break;
	case DRAW_CHAR:
		if (best && best->wdl == current->wdl) {
#ifdef _WIN32 
			SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			printf("%c ", DRAW_CHAR);
			SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
			printf("\e[1;33m%c\e[0m ", DRAW_CHAR);
#endif
		}
		else {
#ifdef _WIN32 
			SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN);
			printf("%c ", DRAW_CHAR);
			SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
			printf("\e[0;33m%c\e[0m ", DRAW_CHAR);
#endif
		}
		break;
	case WIN_CHAR:
		if (current->dt7) {
			if (best && best->dt7 == current->dt7 && best->wdl == current->wdl) {
#ifdef _WIN32 
				SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				printf("%c%d ", current->wdl, current->dt7);
				SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
				printf("\e[1;32m%c%d\e[0m ", current->wdl, current->dt7);
#endif
			}
			else {
#ifdef _WIN32 
				SetConsoleTextAttribute(handle, FOREGROUND_GREEN);
				printf("%c%d ", current->wdl, current->dt7);
				SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
				printf("\e[0;32m%c%d\e[0m ", current->wdl, current->dt7);
#endif
			}
		}
		else {
			if (best && best->dt7 == current->dt7 && best->wdl == current->wdl) {
#ifdef _WIN32
				SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				printf("%s ", WIN_TEXT);
				SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
				printf("\e[1;32m%s\e[0m ", WIN_TEXT);
#endif
			}
			else {
#ifdef _WIN32
				SetConsoleTextAttribute(handle, FOREGROUND_GREEN);
				printf("%s ", WIN_TEXT);
				SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
				printf("\e[0;32m%s\e[0m ", WIN_TEXT);
#endif
			}
		}
		break;
	case LOSS_CHAR:
		if (current->dt7) {
			if (best && best->dt7 == current->dt7 && best->wdl == current->wdl) {
#ifdef _WIN32
				SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
				printf("%c%d ", current->wdl, current->dt7);
				SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
				printf("\e[1;31m%c%d\e[0m ", current->wdl, current->dt7);
#endif
			}
			else {
#ifdef _WIN32
				SetConsoleTextAttribute(handle, FOREGROUND_RED);
				printf("%c%d ", current->wdl, current->dt7);
				SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
				printf("\e[0;31m%c%d\e[0m ", current->wdl, current->dt7);
#endif
			}
		}
		else {
			if (best && best->dt7 == current->dt7 && best->wdl == current->wdl) {
#ifdef _WIN32
				SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
				printf("%s ", LOSS_TEXT);
				SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
				printf("\e[1;31m%s\e[0m ", LOSS_TEXT);
#endif
			}
			else {
#ifdef _WIN32
				SetConsoleTextAttribute(handle, FOREGROUND_RED);
				printf("%s ", LOSS_TEXT);
				SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
				printf("\e[0;31m%s\e[0m ", LOSS_TEXT);
#endif
			}
		}
	}
}

void Result_increment(Result *r) {
	switch (r->wdl) {
	case WIN_CHAR:
		r->wdl = LOSS_CHAR;
		++r->dt7;
		break;
	case LOSS_CHAR:
		r->wdl = WIN_CHAR;
		++r->dt7;
	}
}

Result Result_getBestResult(Result *r1, Result *r2, Result *r3) {
	unsigned i, j, score;
	Result best = { LOSS_CHAR, 0 };
	if (r1 && r2 && r3) {
		for (i = score = 0; i < 3; ++i) {
			for (j = 0; j < 7; ++j) {
				switch (score) {
				case 0:
					switch (r1[i].wdl) {
					case LOSS_CHAR:
						if (best.dt7 < r1[i].dt7) {
							best.dt7 = r1[i].dt7;
						}
						break;
					case DRAW_CHAR:
						best = r1[i];
						score = 1;
						break;
					case WIN_CHAR:
						best = r1[i];
						score = 2;
					}
					switch (r2[i].wdl) {
					case LOSS_CHAR:
						if (best.dt7 < r2[i].dt7) {
							best.dt7 = r2[i].dt7;
						}
						break;
					case DRAW_CHAR:
						best = r2[i];
						score = 1;
						break;
					case WIN_CHAR:
						best = r2[i];
						score = 2;
					}
					switch (r3[i].wdl) {
					case LOSS_CHAR:
						if (best.dt7 < r3[i].dt7) {
							best.dt7 = r3[i].dt7;
						}
						break;
					case DRAW_CHAR:
						best = r3[i];
						score = 1;
						break;
					case WIN_CHAR:
						best = r3[i];
						score = 2;
					}
					break;
				case 1:
					switch (r1[i].wdl) {
					case WIN_CHAR:
						best = r1[i];
						score = 2;
					}
					switch (r2[i].wdl) {
					case WIN_CHAR:
						best = r2[i];
						score = 2;
					}
					switch (r3[i].wdl) {
					case WIN_CHAR:
						best = r3[i];
						score = 2;
					}
					break;
				case 2:
					if (r1[i].wdl == WIN_CHAR && best.dt7 > r1[i].dt7) {
						best.dt7 = r1[i].dt7;
					}
					else if (r2[i].wdl == WIN_CHAR && best.dt7 > r2[i].dt7) {
						best.dt7 = r2[i].dt7;
					}
					else if (r3[i].wdl == WIN_CHAR && best.dt7 > r3[i].dt7) {
						best.dt7 = r3[i].dt7;
					}
				}
			}
		}
	}
	return best;
}

// 
short Result_getBestMove(Result *r1, Result *r2, Result *r3) {
	if (r1 && r2 && r3) {
		int i, j;
		Result bestResult = Result_getBestResult(r1, r2, r3);
		srand(time(NULL));
		for (i = 0; i < 3; ++i) {
			for (j = 0; j < 7; ++j) {
				// fixme:
			}
		}
		return 1;
	}
	return 0;
}
