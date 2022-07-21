/*
	Copyright (C) 2020 TheTrustedComputer
*/

#include "makeseven.h"

uint64_t intpow(uint64_t x, uint64_t y) {
	uint64_t z = 1ull;
	while (y) {
		if (y & 1ull) {
			z *= x;
		}
		x *= x;
		y >>= 1ull;
	}
	return z;
}

int intlog2(uint64_t x) {
	if (!x) {
		return INT_MIN; // lim x->0+ = -inf
	}
	int y = 0;
	while (x > 1ull) {
		x >>= 1ull;
		++y;
	}
	return y;
}

int popcount(uint64_t x) {
	int count = 0;
	while (x) {
		x &= x - 1ull;
		++count;
	}
	return count;
}

void MakeSeven_initialize(MakeSeven *ms) {
	int i;

	ms->playerTiles[0] = ms->playerTiles[1] = 0ull;
	ms->twoAndThreeTiles[0] = ms->twoAndThreeTiles[1] = 0ull;
	ms->remainingTiles[0] = ms->remainingTiles[1] = 0xbb; // Eleven in hexadecimal
	ms->remainingTiles[2] = 0x44;
	ms->plyNumber = 0;
	MakeSeven_inputReadyFlag = 0;
	for (i = 0; i < MAKESEVEN_SIZE; ++i) {
		ms->height[i] = (uint8_t)(MAKESEVEN_SIZE_P1 * i);
	}
}

void MakeSeven_print(const MakeSeven *ms) {
	uint8_t numOneTiles, numTwoTiles, numThreeTiles;
	int i, j, turn = ms->plyNumber & 1;
	uint64_t bit, ONE_TILES = (ms->playerTiles[0] | ms->playerTiles [1]) ^ (ms->twoAndThreeTiles[0] | ms->twoAndThreeTiles[1]);

#ifdef _WIN32
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
	if (turn) {
#ifdef _WIN32
		SetConsoleTextAttribute(handle, 0xe);
#else
		printf("\e[1;93m");
#endif
	}
	else {
#ifdef _WIN32
		SetConsoleTextAttribute(handle, 0xa);
#else
		printf("\e[1;92m");
#endif
	}
	for (i = 0; i < MAKESEVEN_SIZE; ++i) {
		printf("%c ", i + 'A');

	}
#ifdef _WIN32
	SetConsoleTextAttribute(handle, 0x7);
#else
	printf("\e[0m");
#endif
	puts("");
	for (i = MAKESEVEN_SIZE - 1; i >= 0; --i) {
		for (j = 0; j < MAKESEVEN_SIZE; ++j) {
			bit = intpow(2, (uint64_t)(MAKESEVEN_SIZE + 1) * j) * intpow(2, i);
			if (bit & ms->playerTiles[0]) {
				if (bit & ONE_TILES) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xa);
					printf("1 ");
#else
					printf("\e[1;7;92m1\e[0m ");
#endif
				}
				else if (bit & ms->twoAndThreeTiles[0]) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xa);
					printf("2 ");
#else
					printf("\e[1;7;92m2\e[0m ");
#endif
				}
				else {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xa);
					printf("3 ");
#else
					printf("\e[1;7;92m3\e[0m ");
#endif
				}
			}
			else if (bit & ms->playerTiles[1]) {
				if (bit & ONE_TILES) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xe);
					printf("1 ");
#else
					printf("\e[1;7;93m1\e[0m ");
#endif
				}
				else if (bit & ms->twoAndThreeTiles[0]) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xe);
					printf("2 ");
#else
					printf("\e[1;7;93m2\e[0m ");
#endif
				}
				else {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xe);
					printf("3 ");
#else
					printf("\e[1;7;93m3\e[0m ");
#endif
				}
			}
			else {
				if (bit & MAKESEVEN_THREES) {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xc);
					printf(". ");
#else
					printf("\e[91m. \e[0m");
#endif
				}
				else {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, 0xd);
					printf(". ");
#else
					printf("\e[95m. \e[0m");
#endif
				}
			}
		}
		if (i) {
#ifdef _WIN32
			SetConsoleTextAttribute(handle, 0x7);
#else
			printf("\e[0m");
#endif
			puts("");
		}
	}
#ifdef _WIN32
	SetConsoleTextAttribute(handle, 0x7);
#endif
	numOneTiles = turn ? (ms->remainingTiles[0] & 0xf0) >> 4 : ms->remainingTiles[0] & 0xf;
	numTwoTiles = turn ? (ms->remainingTiles[1] & 0xf0) >> 4 : ms->remainingTiles[1] & 0xf;
	numThreeTiles = turn ? (ms->remainingTiles[2] & 0xf0) >> 4 : ms->remainingTiles[2] & 0xf;
	printf("\n1:%u 2:%u 3:%u\n", numOneTiles, numTwoTiles, numThreeTiles);
}

bool MakeSeven_tilesSumToSeven(const MakeSeven *ms) {
	if (ms->plyNumber) {
		uint64_t ALL_TWOTILES_BITMASK, ALL_THREETILES_BITMASK, PLAYER_TILES_BITMASK, PLAYER_ONETILES_BITMASK, PLAYER_TWOTILES_BITMASK, PLAYER_THREETILES_BITMASK, tileBit;
		uint8_t MOVES_HISTORY, PLY_M1, NUMTILE_HEIGHT, NUMTILE_DIGIT, NUMTILE_COLUMN, TURN, shiftSwitcher, bitShifter, sum, currentAdjacent0, currentAdjacent1, adjacentTiles[24];
		int8_t MIN_VERT_BITSHIFT, MAX_VERT_BITSHIFT, MAX_HORI_BITSHIFT, MIN_PSLOPE_BITSHIFT, MAX_PSLOPE_BITSHIFT, MIN_MSLOPE_BITSHIFT, MAX_MSLOPE_BITSHIFT, direction, nextDir, relativeTile, relativeOffset;
		bool finishedLeftOrDown, finishedRightOrUp;

		// Variables initialization
		PLY_M1 = ms->plyNumber - 1u;
		TURN = PLY_M1 & 1u;
		MOVES_HISTORY = ms->movesHistory[PLY_M1];
		NUMTILE_HEIGHT = ms->height[MOVES_HISTORY & 0xf] - 1;
		NUMTILE_DIGIT = (MOVES_HISTORY & 0xf0) >> 4;
		NUMTILE_COLUMN = NUMTILE_HEIGHT / MAKESEVEN_SIZE_P1;
		ALL_TWOTILES_BITMASK = ms->twoAndThreeTiles[0];
		ALL_THREETILES_BITMASK = ms->twoAndThreeTiles[1];
		PLAYER_TILES_BITMASK = ms->playerTiles[TURN];
		PLAYER_ONETILES_BITMASK = PLAYER_TILES_BITMASK ^ (PLAYER_TILES_BITMASK & (ALL_TWOTILES_BITMASK | ALL_THREETILES_BITMASK));
		PLAYER_TWOTILES_BITMASK = PLAYER_TILES_BITMASK & ALL_TWOTILES_BITMASK;
		PLAYER_THREETILES_BITMASK = PLAYER_TILES_BITMASK & ALL_THREETILES_BITMASK;
		shiftSwitcher = 0;

		// Maximum/minimum vertical and horizontal shift boundaries to avoid wraparound
		MIN_VERT_BITSHIFT = NUMTILE_COLUMN * MAKESEVEN_SIZE_P1;
		MAX_VERT_BITSHIFT = (NUMTILE_COLUMN + 1) * MAKESEVEN_SIZE_P1 - 2;
		MAX_HORI_BITSHIFT = 54;

		// The diagonals require slightly more computational work to get bounds correct
		MIN_PSLOPE_BITSHIFT = MAX_PSLOPE_BITSHIFT = MIN_MSLOPE_BITSHIFT = MAX_MSLOPE_BITSHIFT = NUMTILE_HEIGHT;
		while (!((1ull << (MAX_MSLOPE_BITSHIFT + MAKESEVEN_SIZE)) & MAKESEVEN_TOP) && (MAX_MSLOPE_BITSHIFT <= 46)) {
			MAX_MSLOPE_BITSHIFT += MAKESEVEN_SIZE;
		}
		while (!((1ull << (MIN_MSLOPE_BITSHIFT - MAKESEVEN_SIZE)) & MAKESEVEN_TOP) && (MIN_MSLOPE_BITSHIFT >= MAKESEVEN_SIZE)) {
			MIN_MSLOPE_BITSHIFT -= MAKESEVEN_SIZE;
		}
		while (!((1ull << (MAX_PSLOPE_BITSHIFT + MAKESEVEN_SIZE_P2)) & MAKESEVEN_TOP) && (MAX_PSLOPE_BITSHIFT <= 45)) {
			MAX_PSLOPE_BITSHIFT += MAKESEVEN_SIZE_P2;
		}
		while (!((1ull << (MIN_PSLOPE_BITSHIFT - MAKESEVEN_SIZE_P2)) & MAKESEVEN_TOP) && (MIN_PSLOPE_BITSHIFT >= MAKESEVEN_SIZE_P2)) {
			MIN_PSLOPE_BITSHIFT -= MAKESEVEN_SIZE_P2;
		}

		// First adjacent tile is itself
		adjacentTiles[0] = NUMTILE_DIGIT;

		Label_nextDirectionShifter:
		// The program needs to test for all four possible directions to make seven
		switch (shiftSwitcher) {
		case 0:
			bitShifter = 1; // Vertical shift (|)
			break;
		case 1:
			bitShifter = MAKESEVEN_SIZE_P1; // Horizontal shift (-)
			break;
		case 2:
			bitShifter = MAKESEVEN_SIZE; // Negative sloped diagonal shift (\)
			break;
		case 3:
			bitShifter = MAKESEVEN_SIZE_P2; // Positive sloped diagonal shift (/)
		}

		//Reset found adjacent tiles
		for (currentAdjacent0 = 1; currentAdjacent0 < 24; ++currentAdjacent0) {
			adjacentTiles[currentAdjacent0] = 0;
		}

		// Flags to stop after finding any opponent or empty tiles
		finishedLeftOrDown = finishedRightOrUp = false;
		direction = -1;
		currentAdjacent0 = 1;
		relativeOffset = bitShifter * direction;

		// Go left first from the last dropped tile, then go right
		for (nextDir = 0; !(finishedLeftOrDown && finishedRightOrUp) && (nextDir < 6) && (direction <= 1); ++nextDir) {
			relativeTile = NUMTILE_HEIGHT + relativeOffset;
			tileBit = 1ull << relativeTile;

			// Do not go out of bounds
			switch (bitShifter) {
			case 1:
				if (relativeTile < MIN_VERT_BITSHIFT) {
					finishedLeftOrDown = true;
					nextDir = -1;
					goto Label_resetBounds;
				}
				if (relativeTile > MAX_VERT_BITSHIFT) {
					finishedRightOrUp = true;
					goto Label_resetBounds;
				}
				goto Label_tilesSumToSeven;
			case MAKESEVEN_SIZE_P1:
				if (relativeTile < 0) {
					finishedLeftOrDown = true;
					nextDir = -1;
					goto Label_resetBounds;
				}
				if (relativeTile > MAX_HORI_BITSHIFT) {
					finishedRightOrUp = true;
					goto Label_resetBounds;
				}
				goto Label_tilesSumToSeven;
			case MAKESEVEN_SIZE:
				if (relativeTile < MIN_MSLOPE_BITSHIFT) {
					finishedLeftOrDown = true;
					nextDir = -1;
					goto Label_resetBounds;
				}
				if (relativeTile > MAX_MSLOPE_BITSHIFT) {
					finishedRightOrUp = true;
					goto Label_resetBounds;
				}
				goto Label_tilesSumToSeven;
			case MAKESEVEN_SIZE_P2:
				if (relativeTile < MIN_PSLOPE_BITSHIFT) {
					finishedLeftOrDown = true;
					nextDir = -1;
					goto Label_resetBounds;
				}
				if (relativeTile > MAX_PSLOPE_BITSHIFT) {
					finishedRightOrUp = true;
					goto Label_resetBounds;
				}
				goto Label_tilesSumToSeven;
			}

			Label_resetBounds:
			direction += 2;
			relativeOffset = 0;
			goto Label_continueStatement;

			Label_tilesSumToSeven:
			// No exploring after reaching the end
			if (finishedLeftOrDown && (direction == -1)) {
				goto Label_resetBounds;
			}

			// Found one tiles
			if (PLAYER_ONETILES_BITMASK & tileBit) {
				adjacentTiles[currentAdjacent0++] = 1;
			}
			// Found two tiles
			else if (PLAYER_TWOTILES_BITMASK & tileBit) {
				adjacentTiles[currentAdjacent0++] = 2;
			}
			// Found three tiles
			else if (PLAYER_THREETILES_BITMASK & tileBit) {
				adjacentTiles[currentAdjacent0++] = 3;
			}
			else { // Found nothing or opponent tiles
				if (direction == 1) {
					finishedRightOrUp = true;
				}
				else {
					finishedLeftOrDown = true;
					goto Label_resetBounds;
				}
			}

			Label_continueStatement:
			// Increment the bit shifting mechanism
			switch (shiftSwitcher) {
			case 0:
				relativeOffset += 1 * direction;
				break;
			case 1:
				relativeOffset += MAKESEVEN_SIZE_P1 * direction;
				break;
			case 2:
				relativeOffset += MAKESEVEN_SIZE * direction;
				break;
			case 3:
				relativeOffset += MAKESEVEN_SIZE_P2 * direction;
			}
		}

		// Sum all the tiles and see if they "Make 7"
		for (currentAdjacent0 = 0; adjacentTiles[currentAdjacent0] && (currentAdjacent0 < 23); ++currentAdjacent0) {
			sum = adjacentTiles[currentAdjacent0];
			for (currentAdjacent1 = currentAdjacent0 + 1; adjacentTiles[currentAdjacent1] && (sum <= 7) && (currentAdjacent1 < 24); ++currentAdjacent1) {
				if ((sum += adjacentTiles[currentAdjacent1]) == 7) {
					return true;
				}
			}
			sum = 0; // Reset sum to zero when it is not seven
		}

		if ((++shiftSwitcher) <= 3) { // Switch directions and repeat
			goto Label_nextDirectionShifter;
		}
	}
	return false;
}

bool MakeSeven_gameOver(const MakeSeven *ms) {
	return MakeSeven_tilesSumToSeven(ms) || MakeSeven_hasNoMoreMoves(ms);
}

bool MakeSeven_hasNoMoreMoves(const MakeSeven *ms) {
	int i;
	uint8_t remaining[3];

	// Get the number of remaining tiles for that player
	for (i = 0; i < 3; ++i) {
		remaining[i] = ms->plyNumber & 1u ? (ms->remainingTiles[i] & 0xf0) >> 4 : ms->remainingTiles[i] & 0xf;
	}

	// Does the current player has no more 1 and 2 tiles left?
	if (!(remaining[0] || remaining[1])) {
		// The player does not. Does the board state allows droppable the 3 tiles? If so, the player to move is able to drop them in this case.
		return !(remaining[2] && ((ms->playerTiles[0] | ms->playerTiles[1]) + MAKESEVEN_BOT) & MAKESEVEN_THREES);
	}

	// The player has droppable 1 or 2 tiles. Play it through until the former condition is met.
	return false;
}

bool MakeSeven_drop(MakeSeven *ms, const int NUM_TILE, const int COLUMN) {
	// Turn on a single bit that drops a number tile to that column.
	uint64_t droppedTile = 1ull << ms->height[COLUMN];
	int NUM_TILE_M1 = NUM_TILE - 1;
	uint8_t turn = ms->plyNumber & 1u, tileAmount = turn ? (ms->remainingTiles[NUM_TILE_M1] & 0xf0) >> 4 : ms->remainingTiles[NUM_TILE_M1] & 0xf, tileAmountBitmask;

	// Is the column not full of tiles, and does the player have any tiles left?
	if (!(droppedTile & MAKESEVEN_TOP) && tileAmount) {
		if (NUM_TILE == 3) {
			// Are the 3 tiles dropped at their specified locations?
			if (!(droppedTile & MAKESEVEN_THREES)) {
				// The 3 tiles are not dropped to where they're supposed to be.
				return false;
			}
		}

		// This drop is legal. Now bitwise-or it with the bitmap of that player's dropped tiles.
		ms->playerTiles[turn] |= droppedTile;
		// The MakeSeven structure does not have any means of saving one tiles in memory; check if this tile is not a 1 and bitwise-or to the two-and-three tiles variable.
		// If no bit in twoAndThreeTiles are flipped on and there is a bit in playerTiles at the same spot, then it is guaranteed to be a 1 tile.
		if (NUM_TILE_M1) {
			ms->twoAndThreeTiles[NUM_TILE_M1 - 1] |= droppedTile;
		}
		ms->movesHistory[ms->plyNumber++] = COLUMN | (NUM_TILE << 4);												// Store this move to the move variation history.
		++ms->height[COLUMN];																						// Increase the column height where the tile was dropped in.
		tileAmountBitmask = turn ? ms->remainingTiles[NUM_TILE_M1] & 0xf : ms->remainingTiles[NUM_TILE_M1] & 0xf0;	// Save bits containing the amount of opponent's number tiles.
		ms->remainingTiles[NUM_TILE_M1] = tileAmountBitmask | (--tileAmount << (turn << 2));						// Deduct that number's remaining tiles from the given player.
		return true;
	}
	return false;
}

void MakeSeven_undrop(MakeSeven *ms) {
	// Retrieve the last move from the movesHistory array and also retrieve that tile
	uint8_t lastDroppedHeight = ms->movesHistory[--ms->plyNumber] & 0xf, turn = ms->plyNumber & 1u, lastNumberTile, tileAmount, tileAmountBitmask;
	lastNumberTile = ((ms->movesHistory[ms->plyNumber] & 0xf0) >> 4) - 1;
	ms->playerTiles[turn] ^= 1ull << --ms->height[lastDroppedHeight];

	// Do not turn off this bit if the last tile played is a 1 tile
	if (lastNumberTile) {
		ms->twoAndThreeTiles[lastNumberTile - 1] ^= 1ull << ms->height[lastDroppedHeight];
	}

	// Reverse the drop move to the state before it occurred
	tileAmount = turn ? (ms->remainingTiles[lastNumberTile] & 0xf0) >> 4 : ms->remainingTiles[lastNumberTile] & 0xf;
	tileAmountBitmask = turn ? ms->remainingTiles[lastNumberTile] & 0xf : ms->remainingTiles[lastNumberTile] & 0xf0;
	ms->remainingTiles[lastNumberTile] = tileAmountBitmask | (++tileAmount << (turn << 2));
}

bool MakeSeven_getUserCharInput(MakeSeven *ms, const char INPUT) {
	int number = INPUT - '0', column = INPUT - 'A', caseTest;
	if (MakeSeven_inputReadyFlag) {
		// Use case-insensitive input
		for (caseTest = 0; caseTest < 2; ++caseTest) {
			if (column >= 0 && column < MAKESEVEN_SIZE) {
				MakeSeven_inputReadyFlag = 0;
				return MakeSeven_drop(ms, MakeSeven_userNumberTile, column);
			}
			column = INPUT - 'a';
		}
	}
	else {
	    if (number > 0 && number <= 3) {
			MakeSeven_userNumberTile = number;
			MakeSeven_inputReadyFlag = 1;
			return true;
	    }
	}
	return false;
}

bool MakeSeven_sequence(MakeSeven *ms, const char *SEQ) {
	for (int i = 0; SEQ[i]; ++i) {
		if (!MakeSeven_getUserCharInput(ms, SEQ[i])) {
			return false;
		}
	}
	return true;
}

uint64_t MakeSeven_hashEncode(MakeSeven *ms) {
	return ms->playerTiles[ms->plyNumber & 1u] + ms->playerTiles[0] + ms->playerTiles[1] + MAKESEVEN_BOT;
}
