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
	while (x >>= 1ull) {
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
	
#ifdef _WIN32  // Get console handle
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
	if (turn) { // Colorize coordinates on whose turn it is
#ifdef _WIN32
		SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
		printf("\e[1;93m");
#endif
	}
	else {
#ifdef _WIN32
		SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
		printf("\e[1;92m");
#endif
	}
	// Print coordinates
	for (i = 0; i < MAKESEVEN_SIZE; ++i) {
		printf("%c ", i + 'A');
	}
#ifdef _WIN32
	SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
	printf("\e[0m");
#endif
	puts("");
	
	// Print playing grid
	for (i = MAKESEVEN_SIZE - 1; i >= 0; --i) {
		for (j = 0; j < MAKESEVEN_SIZE; ++j) {
			// Compute tile bit position
			bit = ((uint64_t)(1ul) << (MAKESEVEN_SIZE_P1 * j)) * ((uint64_t)(1ul) << i); 
			if (bit & ms->playerTiles[0]) { // Green's tiles
				if (bit & ONE_TILES) { // 1 tile
#ifdef _WIN32
					SetConsoleTextAttribute(handle, BACKGROUND_GREEN | BACKGROUND_INTENSITY);
					printf("1");
					SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
					printf(" ");
#else
					printf("\e[1;7;92m1\e[0m ");
#endif
				}
				else if (bit & ms->twoAndThreeTiles[0]) { // 2 tile
#ifdef _WIN32
					SetConsoleTextAttribute(handle, BACKGROUND_GREEN | BACKGROUND_INTENSITY);
					printf("2");
					SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
					printf(" ");
#else
					printf("\e[1;7;92m2\e[0m ");
#endif
				}
				else { // 3 tile
#ifdef _WIN32
					SetConsoleTextAttribute(handle, BACKGROUND_GREEN | BACKGROUND_INTENSITY);
					printf("3");
					SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
					printf(" ");
#else
					printf("\e[1;7;92m3\e[0m ");
#endif
				}
			}
			else if (bit & ms->playerTiles[1]) { // Yellow's tiles
				if (bit & ONE_TILES) { // 1 tile
#ifdef _WIN32
					SetConsoleTextAttribute(handle, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY);
					printf("1");
					SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
					printf(" ");
#else
					printf("\e[1;7;93m1\e[0m ");
#endif
				}
				else if (bit & ms->twoAndThreeTiles[0]) { // 2 tile
#ifdef _WIN32
					SetConsoleTextAttribute(handle, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY);
					printf("2");
					SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
					printf(" ");
#else
					printf("\e[1;7;93m2\e[0m ");
#endif
				}
				else { // 3 tile
#ifdef _WIN32
					SetConsoleTextAttribute(handle, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY);
					printf("3");
					SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
					printf(" ");
#else
					printf("\e[1;7;93m3\e[0m ");
#endif
				}
			}
			else {
				if (bit & MAKESEVEN_THREES) { // Simulate red stickers on cells where 3 tiles are supposed to drop
#ifdef _WIN32
					SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
					printf("= ");
#else
					printf("\e[91m= \e[0m");
#endif
				}
				else {
#ifdef _WIN32
					SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
					printf("- ");
#else
					printf("\e[95m- \e[0m");
#endif
				}
			}
		}
		if (i) {
#ifdef _WIN32
			SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
			printf("\e[0m");
#endif
			puts("");
		}
	}
#ifdef _WIN32
	SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
	// Print remaining tiles
	numOneTiles = turn ? (ms->remainingTiles[0] >> 4) : (ms->remainingTiles[0] & 0xf);
	numTwoTiles = turn ? (ms->remainingTiles[1] >> 4) : (ms->remainingTiles[1] & 0xf);
	numThreeTiles = turn ? (ms->remainingTiles[2] >> 4) : (ms->remainingTiles[2] & 0xf);
	printf("\n1:%u 2:%u 3:%u\n", numOneTiles, numTwoTiles, numThreeTiles);
}

bool MakeSeven_tilesSumToSeven(const MakeSeven *ms) {
	if (ms->plyNumber) { // TODO: Remove this if statement; it crashes on empty board without
		uint64_t ALL_TWOTILES_BITMASK, ALL_THREETILES_BITMASK, PLAYER_TILES_BITMASK, PLAYER_ONETILES_BITMASK, PLAYER_TWOTILES_BITMASK, PLAYER_THREETILES_BITMASK, CURRENT_ONES_BITMASK, CURRENT_TWOS_BITMASK, CURRENT_THREES_BITMASK, CURRENT_ALL_BITMASK, HORI_BOUND_BITMASK, tileBit;
		uint8_t totalTiles, adjacentTiles[MAKESEVEN_SIZE], winSum, winStart, winEnd, MOVES_HISTORY, PLY_M1, NUMTILE_HEIGHT, TURN, shiftSwitcher, bitShifter;
		
		// Variables initialization
		PLY_M1 = ms->plyNumber - 1;
		TURN = PLY_M1 & 1;
		MOVES_HISTORY = ms->movesHistory[PLY_M1];
		NUMTILE_HEIGHT = ms->height[MOVES_HISTORY & 0xf] - 1;
		ALL_TWOTILES_BITMASK = ms->twoAndThreeTiles[0];
		ALL_THREETILES_BITMASK = ms->twoAndThreeTiles[1];
		PLAYER_TILES_BITMASK = ms->playerTiles[TURN];
		PLAYER_ONETILES_BITMASK = PLAYER_TILES_BITMASK ^ (PLAYER_TILES_BITMASK & (ALL_TWOTILES_BITMASK | ALL_THREETILES_BITMASK));
		PLAYER_TWOTILES_BITMASK = PLAYER_TILES_BITMASK & ALL_TWOTILES_BITMASK;
		PLAYER_THREETILES_BITMASK = PLAYER_TILES_BITMASK & ALL_THREETILES_BITMASK;
		HORI_BOUND_BITMASK = MAKESEVEN_BOT << (NUMTILE_HEIGHT % MAKESEVEN_SIZE_P1);
		shiftSwitcher = 0;
		
		Label_nextDirectionShifter:
		// Test all four directions.
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
		
		// (Re)set looping variables
		totalTiles = 0;
		tileBit = 1ull << NUMTILE_HEIGHT;
		
		// Set bitmasks for potential wins per direction
		switch (bitShifter) {
		case 1:
			CURRENT_ONES_BITMASK = PLAYER_ONETILES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
			CURRENT_TWOS_BITMASK = PLAYER_TWOTILES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
			CURRENT_THREES_BITMASK = PLAYER_THREETILES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
			CURRENT_ALL_BITMASK = PLAYER_TILES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];	
			break;
		case MAKESEVEN_SIZE_P1:
			CURRENT_ONES_BITMASK = PLAYER_ONETILES_BITMASK & HORI_BOUND_BITMASK;
			CURRENT_TWOS_BITMASK = PLAYER_TWOTILES_BITMASK & HORI_BOUND_BITMASK;
			CURRENT_THREES_BITMASK = PLAYER_THREETILES_BITMASK & HORI_BOUND_BITMASK;
			CURRENT_ALL_BITMASK = PLAYER_TILES_BITMASK & HORI_BOUND_BITMASK;
			break;
		case MAKESEVEN_SIZE:
			CURRENT_ONES_BITMASK = PLAYER_ONETILES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
			CURRENT_TWOS_BITMASK = PLAYER_TWOTILES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
			CURRENT_THREES_BITMASK = PLAYER_THREETILES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
			CURRENT_ALL_BITMASK = PLAYER_TILES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
			break;
		case MAKESEVEN_SIZE_P2:
			CURRENT_ONES_BITMASK = PLAYER_ONETILES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
			CURRENT_TWOS_BITMASK = PLAYER_TWOTILES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
			CURRENT_THREES_BITMASK = PLAYER_THREETILES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
			CURRENT_ALL_BITMASK = PLAYER_TILES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
		}
		
		// Move tileBit to the leftmost tile before hitting an empty tile
		if (bitShifter != 1) {
			while (CURRENT_ALL_BITMASK & (tileBit >> bitShifter)) {
				tileBit >>= bitShifter;
			}
		}
		
		// Search tiles in this direction and store them in the adjacentTiles array
		while (CURRENT_ALL_BITMASK) {
			if (CURRENT_ONES_BITMASK & tileBit) {
				adjacentTiles[totalTiles++] = 1; // Found one tiles
			}	
			else if (CURRENT_TWOS_BITMASK & tileBit) {
				adjacentTiles[totalTiles++] = 2; // Found two tiles
			}
			else if (CURRENT_THREES_BITMASK & tileBit) {
				adjacentTiles[totalTiles++] = 3; // Found three tiles
			}
			
			// Shift toward zero to find the next tile.
			CURRENT_ONES_BITMASK = (bitShifter == 1) ? (CURRENT_ONES_BITMASK << bitShifter) : (CURRENT_ONES_BITMASK >> bitShifter);
			CURRENT_TWOS_BITMASK = (bitShifter == 1) ? (CURRENT_TWOS_BITMASK << bitShifter) : (CURRENT_TWOS_BITMASK >> bitShifter);
			CURRENT_THREES_BITMASK = (bitShifter == 1) ? (CURRENT_THREES_BITMASK << bitShifter) : (CURRENT_THREES_BITMASK >> bitShifter);
			CURRENT_ALL_BITMASK &= (bitShifter == 1) ? (CURRENT_ALL_BITMASK << bitShifter) : (CURRENT_ALL_BITMASK >> bitShifter);
		}
		
		// Miniumum number of adjacent tiles is 3 since this is smallest quantity of tiles that can add to seven.
		if (totalTiles >= 3) { 
			// Sum all found adjacent tiles and see if they "Make 7".
			// There are eight ways of adding to 7 given numbers 1, 2, and 3:
			//
			// 1. 3+3+1 = 7
			// 2. 3+2+2 = 7
			// 3. 3+2+1+1 = 7
			// 4. 3+1+1+1+1 = 7
			// 5. 2+2+2+1 = 7
			// 6. 2+2+1+1+1 = 7
			// 7. 2+1+1+1+1+1 = 7
			// 8. 1+1+1+1+1+1+1 = 7
			//
			// Addition is commutative, so they can be in any order: 3+3+1 = 3+1+3 = 1+3+3 = 7.
			// Partial sums are allowed as long they are in sequence: 3+3+1+2 is a winner, but 3+3+2+1 is not!
			//
			// Below is a variant of the sliding window algorithm with increasing window size when the window sum is greater than 7.
			// Profiling showed this to be roughly 5% faster while exploring nearly the same number of positions than caching sums to variables.
			for (winSum = adjacentTiles[0], winStart = winEnd = 0;;) {
				if (winSum == 7) {
					return true; // The current player wins.
				}
				if (winSum > 7) {
					winSum -= adjacentTiles[winStart++];
				}
				else if (++winEnd < totalTiles) {
					winSum += adjacentTiles[winEnd];
				}
				else {
					break; // The player to move does not have a "Make 7".
				}
			}
		}
			
		// Switch directions and repeat.
		if ((++shiftSwitcher) <= 3) {
			goto Label_nextDirectionShifter;
		}
	}
	
	// Either no tiles made seven, or the grid was empty.
	return false;
}

bool MakeSeven_gameOver(const MakeSeven *ms) {
	return MakeSeven_tilesSumToSeven(ms) || MakeSeven_hasNoMoreMoves(ms);
}

bool MakeSeven_hasNoMoreMoves(const MakeSeven *ms) {
	uint8_t remaining[3], TURN = ms->plyNumber & 1;
	
	// Get the number of remaining tiles for that player; loop is unrolled.
	remaining[0] = TURN ? (ms->remainingTiles[0] >> 4) : (ms->remainingTiles[0] & 0xf);
	remaining[1] = TURN ? (ms->remainingTiles[1] >> 4) : (ms->remainingTiles[1] & 0xf);
	remaining[2] = TURN ? (ms->remainingTiles[2] >> 4) : (ms->remainingTiles[2] & 0xf);

	// Does the current player has 1 and 2 tiles left?
	// Yes: Play it through until the latter condition from below is met.
	// No: Does the board state allows droppable 3 tiles? If so, the player to move is able to drop them in this case.
	return (remaining[0] || remaining[1]) ? false : !(remaining[2] && ((ms->playerTiles[0] | ms->playerTiles[1]) + MAKESEVEN_BOT) & MAKESEVEN_THREES);
}

bool MakeSeven_drop(MakeSeven *ms, const int NUM_TILE, const int COLUMN) {
	// Turn on a single bit that drops a number tile to that column.
	uint64_t droppedTile = (uint64_t)(1ul) << ms->height[COLUMN];
	int NUM_TILE_M1 = NUM_TILE - 1, NUM_TILE_M2 = NUM_TILE - 2;
	uint8_t TURN = ms->plyNumber & 1, tileAmount = TURN ? (ms->remainingTiles[NUM_TILE_M1] >> 4) : (ms->remainingTiles[NUM_TILE_M1] & 0xf);

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
		ms->playerTiles[TURN] |= droppedTile;
		
		// The MakeSeven structure does not have any means of saving one tiles in memory; check if this tile is not a 1 and bitwise-or to the two-and-three tiles variable.
		// If no bit in twoAndThreeTiles is flipped on and there is a bit in playerTiles at the same spot, then it is guaranteed to be a 1 tile.
		if (NUM_TILE_M1) { 
			ms->twoAndThreeTiles[NUM_TILE_M2] |= droppedTile;
		}

		// Store this move to the move variation history.
		ms->movesHistory[ms->plyNumber++] = COLUMN | (NUM_TILE << 4);
		// Increase the column height where the tile was dropped in.
		++ms->height[COLUMN];
		// Deduct that number's remaining tiles from the given player.
		ms->remainingTiles[NUM_TILE_M1] = (TURN ? (ms->remainingTiles[NUM_TILE_M1] & 0xf) : (ms->remainingTiles[NUM_TILE_M1] & 0xf0)) | (--tileAmount << (TURN << 2));

		// Successful drop.
		return true;
	}

	// Illegal drop.
	return false;
}

void MakeSeven_undrop(MakeSeven *ms) {
	// Retrieve the last move from the movesHistory array and also retrieve that tile
	uint8_t lastDroppedHeight = ms->movesHistory[--ms->plyNumber] & 0xf, turn = ms->plyNumber & 1, lastNumberTile, numberTileM1, tileAmount;
	numberTileM1 = (lastNumberTile = (ms->movesHistory[ms->plyNumber] >> 4) - 1) - 1;
	ms->playerTiles[turn] ^= 1ull << --ms->height[lastDroppedHeight];

	// Do not turn off this bit if the last tile played is a 1 tile
	if (lastNumberTile) {
		ms->twoAndThreeTiles[numberTileM1] ^= 1ull << ms->height[lastDroppedHeight];
	}

	// Reverse the drop move to the state before it occurred
	tileAmount = turn ? (ms->remainingTiles[lastNumberTile] >> 4) : (ms->remainingTiles[lastNumberTile] & 0xf);
	ms->remainingTiles[lastNumberTile] = (turn ? (ms->remainingTiles[lastNumberTile] & 0xf) : (ms->remainingTiles[lastNumberTile] & 0xf0)) | (++tileAmount << (turn << 2));
}

bool MakeSeven_getUserCharInput(MakeSeven *ms, const char INPUT) {
	int number = INPUT - '0', column = INPUT - 'A', caseTest; // Uppercase
	if (!MakeSeven_gameOver(ms)) {
		if (MakeSeven_inputReadyFlag) {
			// Use case-insensitive input
			for (caseTest = 0; caseTest < 2; ++caseTest) {
				if (column >= 0 && column < MAKESEVEN_SIZE) {
					MakeSeven_inputReadyFlag = 0;
					return MakeSeven_drop(ms, MakeSeven_userNumberTile, column);
				}
				column = INPUT - 'a'; // Lowercase
			}
		}
		else {
			// Check tile number input range
			if ((number > 0) && (number <= 3)) { 
				MakeSeven_userNumberTile = number;
				MakeSeven_inputReadyFlag = 1;
				return true;
			}
		}
	}
	return false;
}

bool MakeSeven_sequence(MakeSeven *ms, const char *SEQ) {
	for (int s = 0; SEQ[s]; ++s) {
		if (!MakeSeven_getUserCharInput(ms, SEQ[s])) {
			return false;
		}
	}
	return true;
}

uint64_t MakeSeven_hashEncode(MakeSeven *ms) {
	return ms->playerTiles[ms->plyNumber & 1u] + ms->playerTiles[0] + ms->playerTiles[1] + MAKESEVEN_BOT;
}

uint64_t MakeSeven_reverse(uint64_t grid) {
	uint64_t revGrid, revCols;
	for (revGrid = 0ull, revCols = MAKESEVEN_SIZE_M1; grid; grid >>= MAKESEVEN_SIZE_P1, --revCols) {
		revGrid |= (grid & MAKESEVEN_LCL) << (revCols * MAKESEVEN_SIZE_P1);
	}
	return revGrid;
}

bool MakeSeven_symmetrical(const MakeSeven *ms) {
	uint64_t revTiles[4] = { ms->playerTiles[0], ms->playerTiles[1], ms->twoAndThreeTiles[0], ms->twoAndThreeTiles[1] }, r;
	// Reverse player tiles and number tiles
	for (r = 0; r < 4; ++r) {
		revTiles[r] = MakeSeven_reverse(revTiles[r]);
	}
	// Test original and reversed to see if they are identical
	return (revTiles[0] == ms->playerTiles[0]) && (revTiles[1] == ms->playerTiles[1]) && (revTiles[2] == ms->twoAndThreeTiles[0]) && (revTiles[3] == ms->twoAndThreeTiles[1]);
}
