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
	if (ms->plyNumber) {
		uint64_t ALL_TWOTILES_BITMASK, ALL_THREETILES_BITMASK, PLAYER_TILES_BITMASK, PLAYER_ONETILES_BITMASK, PLAYER_TWOTILES_BITMASK, PLAYER_THREETILES_BITMASK, tileBit;
		int currentAdjacent, adjacentsLD, adjacentsRU, totalTiles, adjacentLeftDown[MAKESEVEN_SIZE_M1], adjacentRightUp[MAKESEVEN_SIZE_M1], adjacentTiles[MAKESEVEN_SIZE], sum012, sum12, sum123, sum2, sum3, sum34, sum345, sum3456, sum4, sum6;
		uint8_t MOVES_HISTORY, PLY_M1, NUMTILE_HEIGHT, NUMTILE_DIGIT, NUMTILE_COLUMN, TURN, shiftSwitcher, bitShifter;
		int8_t MIN_VERT_BITSHIFT, MIN_MSLOPE_BITSHIFT, MAX_MSLOPE_BITSHIFT, MIN_PSLOPE_BITSHIFT, MAX_PSLOPE_BITSHIFT, direction, relativeTile, relativeOffset;
		bool finishedLeftOrDown;
		
		// Variables initialization
		PLY_M1 = ms->plyNumber - 1;
		TURN = PLY_M1 & 1;
		MOVES_HISTORY = ms->movesHistory[PLY_M1];
		NUMTILE_HEIGHT = ms->height[MOVES_HISTORY & 0xf] - 1;
		NUMTILE_DIGIT = MOVES_HISTORY >> 4;
		NUMTILE_COLUMN = NUMTILE_HEIGHT / MAKESEVEN_SIZE_P1;
		ALL_TWOTILES_BITMASK = ms->twoAndThreeTiles[0];
		ALL_THREETILES_BITMASK = ms->twoAndThreeTiles[1];
		PLAYER_TILES_BITMASK = ms->playerTiles[TURN];
		PLAYER_ONETILES_BITMASK = PLAYER_TILES_BITMASK ^ (PLAYER_TILES_BITMASK & (ALL_TWOTILES_BITMASK | ALL_THREETILES_BITMASK));
		PLAYER_TWOTILES_BITMASK = PLAYER_TILES_BITMASK & ALL_TWOTILES_BITMASK;
		PLAYER_THREETILES_BITMASK = PLAYER_TILES_BITMASK & ALL_THREETILES_BITMASK;
		MIN_VERT_BITSHIFT = NUMTILE_COLUMN * MAKESEVEN_SIZE_P1;
		MIN_MSLOPE_BITSHIFT = MIN_MSLOPE_BITSHIFT_TABLE[NUMTILE_HEIGHT];
		MAX_MSLOPE_BITSHIFT = MAX_MSLOPE_BITSHIFT_TABLE[NUMTILE_HEIGHT];
		MIN_PSLOPE_BITSHIFT = MIN_PSLOPE_BITSHIFT_TABLE[NUMTILE_HEIGHT];
		MAX_PSLOPE_BITSHIFT = MAX_PSLOPE_BITSHIFT_TABLE[NUMTILE_HEIGHT];
		shiftSwitcher = 0;
		adjacentTiles[0] = NUMTILE_DIGIT;
		
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
		finishedLeftOrDown = false;
		adjacentsLD = adjacentsRU = direction = -1;
		totalTiles = 1;
		relativeOffset = 0;
		
		// Find all adjacent tiles and store them in the adjacentTiles array.
		for (;;) {
			// Get tile bit and relative tile offset
			tileBit = 1ull << (relativeTile = NUMTILE_HEIGHT + (relativeOffset += bitShifter * direction));
			
			// Do not go out of bounds; break out of the loop after searching in all directions.
			switch (bitShifter) {
			case 1:
				// The last dropped tile is always on top of another and nothing is above it; therefore, there is no need to check tiles above it.
				if (finishedLeftOrDown || (relativeTile < MIN_VERT_BITSHIFT)) {
					goto Label_endTilesLoop;
				}
				goto Label_tilesSumToSeven;
			case MAKESEVEN_SIZE_P1:
				if (relativeTile < 0) { // Min horizontal shift
					break;
				}
				if (relativeTile > 54) { // Max horizontal shift
					goto Label_endTilesLoop;
				}
				goto Label_tilesSumToSeven;
			case MAKESEVEN_SIZE:
				// Computing maximum and minimum diagonal shift bounds takes O(n) time with O(1) space, where n is the number of cells in that direction.
				// Using a precomputed table allowed a 25% decrease in solving time that takes O(1) time but with O(n) space.
				if (relativeTile < MIN_MSLOPE_BITSHIFT) {
					break;
				}
				if (relativeTile > MAX_MSLOPE_BITSHIFT) {
					goto Label_endTilesLoop;
				}
				goto Label_tilesSumToSeven;
			case MAKESEVEN_SIZE_P2:
				if (relativeTile < MIN_PSLOPE_BITSHIFT) {
					break;
				}
				if (relativeTile > MAX_PSLOPE_BITSHIFT) {
					goto Label_endTilesLoop;
				}
				goto Label_tilesSumToSeven;
			}
			
			finishedLeftOrDown = true;
			// Start going in the opposite direction.
			Label_goRightOrUp:
			direction += 2;
			relativeOffset = 0;
			continue;
			
			Label_tilesSumToSeven:
			if (PLAYER_ONETILES_BITMASK & tileBit) { // Found one tiles
				(direction == 1) ? (adjacentRightUp[++adjacentsRU] = 1) : (adjacentLeftDown[++adjacentsLD] = 1);
				++totalTiles;
			}	
			else if (PLAYER_TWOTILES_BITMASK & tileBit) { // Found two tiles
				(direction == 1) ? (adjacentRightUp[++adjacentsRU] = 2) : (adjacentLeftDown[++adjacentsLD] = 2);
				++totalTiles;
			}
			else if (PLAYER_THREETILES_BITMASK & tileBit) { // Found three tiles
				(direction == 1) ? (adjacentRightUp[++adjacentsRU] = 3) : (adjacentLeftDown[++adjacentsLD] = 3);
				++totalTiles;
			}
			else { // Found nothing or opponent tiles
				if (direction == 1) {
					break; // Finished going right or up; break out of the loop.
				}
				else {
					finishedLeftOrDown = true;
					goto Label_goRightOrUp;
				}
			}
		}
		
		Label_endTilesLoop:
		// Miniumum number of adjacent tiles is 3 since this is smallest quantity of tiles that can add to seven.
		if (totalTiles >= 3) {
			// Copy found adjacent tiles from both directions into the adjacentTiles array in left to right fashion.
			for (currentAdjacent = 0; adjacentsLD >= 0; ++currentAdjacent) {
				adjacentTiles[currentAdjacent] = adjacentLeftDown[adjacentsLD--];
			}
			for (adjacentTiles[currentAdjacent++] = NUMTILE_DIGIT, adjacentsRU = 0; currentAdjacent < totalTiles; ++currentAdjacent) {
				adjacentTiles[currentAdjacent] = adjacentRightUp[adjacentsRU++];
			}
			
			// Remaining adjacent tiles are set to zeros as they were not found during the search.
			while (currentAdjacent < MAKESEVEN_SIZE) {
				adjacentTiles[currentAdjacent++] = 0;
			}
			
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
			// This would have been in a loop but is unrolled to maximize performance and skip unnecessary checks.
			// The sums are calculated then stored to avoid redundant additions that appear more than once.
			//
			// 3 adjacent tiles
			// 3+3+1 3+2+2
			// [1][2][3][4]: All matching tiles equal target sum: [3+3+1], [2+2+3]
			//  ^  ^  ^
			if ((sum012 = adjacentTiles[0] + (sum12 = adjacentTiles[1] + (sum2 = adjacentTiles[2]))) == 7) {
				return true;
			}
			// [1][2][3][4]: One non-matching tile: 2+[3+3+1], 1+[2+2+3], 2+[2+2+3]
			//     ^  ^  ^
			if ((sum123 = sum12 + (sum3 = adjacentTiles[3])) == 7) { 
				return true;
			}
			
			// 4 adjacent tiles
			// 3+2+1+1 2+2+2+1
			// [1][2][3][4][5][6][7]: [2+2+2+1]
			//  ^  ^  ^  ^
			if ((sum012 + sum3) == 7) {
				return true;
			}
			// [1][2][3][4][5][6][7]: 2+[2+2+2+1]
			//     ^  ^  ^  ^
			if ((sum123 + (sum4 = adjacentTiles[4])) == 7) {
				return true;
			}
			// [1][2][3][4][5][6][7]: 2+2+[2+2+2+1], 3+1+[2+2+2+1]
			//        ^  ^  ^  ^
			if ((sum2 + (sum345 = (sum34 = sum3 + sum4) + adjacentTiles[5])) == 7) {
				return true;
			}
			// [1][2][3][4][5][6][7]: 2+2+2+[2+2+2+1]
			//           ^  ^  ^  ^
			if ((sum345 + (sum6 = adjacentTiles[6])) == 7) {
				return true;
			}
			
			// 5 adjacent tiles
			// 3+1+1+1+1; 2+2+1+1+1
			// [1][2][3][4][5][6]: [3+1+1+1+1], [2+2+1+1+1]
			//  ^  ^  ^  ^  ^
			if ((sum012 + sum34) == 7) {
				return true;
			}
			// [1][2][3][4][5][6]: 1+[2+1+1+1+2], 1+[1+2+1+1+2], 1+[1+1+2+1+2], 1+[1+1+1+2+2]
			//     ^  ^  ^  ^  ^
			if ((sum12 + sum345) == 7) {
				return true;
			}
			
			// 6 adjacent tiles
			// 2+1+1+1+1+1
			// [1][2][3][4][5][6][7]: [2+1+1+1+1+1]
			//  ^  ^  ^  ^  ^  ^
			if ((sum012 + sum345) == 7) {
				return true;
			}
			// [1][2][3][4][5][6][7]: 1+[1+1+1+1+1+2]
			//     ^  ^  ^  ^  ^  ^
			if ((sum12 + (sum3456 = sum345 + sum6)) == 7) {
				return true;
			}
			
			// 7 adjacent tiles
			// [1][2][3][4][5][6][7]: [1+1+1+1+1+1+1]
			//  ^  ^  ^  ^  ^  ^  ^
			if ((sum012 + sum3456) == 7) {
				return true;
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
		// If no bit in twoAndThreeTiles are flipped on and there is a bit in playerTiles at the same spot, then it is guaranteed to be a 1 tile.
		if (NUM_TILE_M1) { 
			ms->twoAndThreeTiles[NUM_TILE_M2] |= droppedTile;
		}

		// Store this move to the move variation history.
		ms->movesHistory[ms->plyNumber++] = COLUMN | (NUM_TILE << 4);
		// Increase the column height where the tile was dropped in.
		++ms->height[COLUMN];
		// Deduct that number's remaining tiles from the given player.
		ms->remainingTiles[NUM_TILE_M1] = (TURN ? (ms->remainingTiles[NUM_TILE_M1] & 0xf) : (ms->remainingTiles[NUM_TILE_M1] & 0xf0)) | (--tileAmount << (TURN << 2));

		// Sucessful drop.
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