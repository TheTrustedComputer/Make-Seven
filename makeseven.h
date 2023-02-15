/*
	Copyright (C) 2020 TheTrustedComputer

	This is a computer implementation of Make Seven, a Connect Four-like game involving elementary additon.
	The objective of this game is to drop numbers that add up to seven in a vertical, horizontal, or diagonal matter.

	Make Seven ships 11 one tiles, 11 two tiles, and 4 three tiles per player, totaling 52 tiles.
	This structure enforces them so that when any player runs out of tiles, they may not drop any more of them.

	Below is a visual illustration on how the playing board is represented internally inside this data structure:

	Playing board           Player 1        Player 2       One tiles*     Two tiles      Three tiles
	 .  .  .  .  .  .  .    0 0 0 0 0 0 0   0 0 0 0 0 0 0  0 0 0 0 0 0 0  0 0 0 0 0 0 0  0 0 0 0 0 0 0
	 .  .  *  .  *  .  .    0 0 0 0 0 0 0   0 0 0 0 0 0 0  0 0 0 0 0 0 0  0 0 0 0 0 0 0  0 0 0 0 0 0 0
	 .  *  . (2) .  *  .    0 0 0 0 0 0 0   0 0 0 1 0 0 0  0 0 0 0 0 0 0  0 0 0 1 0 0 0  0 0 0 0 0 0 0
	 .  .  . (3) .  .  .    0 0 0 0 0 0 0   0 0 0 1 0 0 0  0 0 0 0 0 0 0  0 0 0 0 0 0 0  0 0 0 1 0 0 0
	 *  .  . (1)[1] . (3)   0 0 0 0 1 0 0   0 0 0 1 0 0 1  0 0 0 1 1 0 0  0 0 0 0 0 0 0  0 0 0 0 0 0 1
	 .  . [2](1)[2] . [1]   0 0 1 0 1 0 1   0 0 0 1 0 0 0  0 0 0 1 0 0 1  0 0 1 0 1 0 0  0 0 0 0 0 0 0
	 . (2)[1][2][1](1)[2]   0 0 1 1 1 0 1   0 1 0 0 0 1 0  0 0 1 0 1 1 0  0 1 0 1 0 0 1  0 0 0 0 0 0 0

	Sym   Description       * Storing one tiles are not required and can be computed with this statement:
	[1] | Player 1's tiles  * One_Tiles = (Player1_Tiles | Player2_Tiles) ^ (Two_Tiles | Three_Tiles);
	(1) | Player 2's tiles
	 .  | Empty (1, 2)
	 *  | Empty (1, 2, 3)
*/

// Header guards
#ifndef MAKESEVEN_H
#define MAKESEVEN_H

// Make Seven's board dimensions is a fixed seven-by-seven square to take into account of the one tiles.
#define MAKESEVEN_SIZE 7
#define MAKESEVEN_SIZE_M1 6
#define MAKESEVEN_SIZE_P1 8
#define MAKESEVEN_SIZE_P2 9
#define MAKESEVEN_AREA 49

// Make Seven static bitmaps; it works pretty much more or less the same as any Connect Four implementation.
#define MAKESEVEN_ALL 0x7f7f7f7f7f7f7full
#define MAKESEVEN_BOT 0x1010101010101ull
#define MAKESEVEN_TOP 0x80808080808080ull
#define MAKESEVEN_LCL 0x7full

// In the physical game, the three tiles can only be dropped at the marked red squares; ones and twos can drop anywhere.
#define MAKESEVEN_THREES 0x4102008201004ull

// The players are named Green and Yellow after the included colored tiles for convenience.
#define MAKESEVEN_PLAYER1_NAME "Green"
#define MAKESEVEN_PLAYER2_NAME "Yellow"

// Temporary storage locations or buffers for the number tiles from user input
static int MakeSeven_userNumberTile, MakeSeven_inputReadyFlag;

// Move variation to store the best move given the root position (Left four bits = Tile #; Right four bits = Column #)
static uint8_t MakeSeven_variation[MAKESEVEN_AREA];

// Vertical bitmask table to search for vertical connections below this tile, including itself
const uint64_t VERT_BITMASK_TABLE[55] = {0x1ull, 0x3ull, 0x7ull, 0xfull, 0x1full, 0x3full, 0x7full, 0x0ull,
										0x100ull, 0x300ull, 0x700ull, 0xf00ull, 0x1f00ull, 0x3f00ull, 0x7f00ull, 0x0ull,
										0x10000ull, 0x30000ull, 0x70000ull, 0xf0000ull, 0x1f0000ull, 0x3f0000ull, 0x7f0000ull, 0x0ull,
										0x1000000ull, 0x3000000ull, 0x7000000ull, 0xf000000ull, 0x1f000000ull, 0x3f000000ull, 0x7f000000ull, 0x0ull,
										0x100000000ull, 0x300000000ull, 0x700000000ull, 0xf00000000ull, 0x1f00000000ull, 0x3f00000000ull, 0x7f00000000ull, 0x0ull,
										0x10000000000ull, 0x30000000000ull, 0x70000000000ull, 0xf0000000000ull, 0x1f0000000000ull, 0x3f0000000000ull, 0x7f0000000000ull, 0x0ull,
										0x1000000000000ull, 0x3000000000000ull, 0x7000000000000ull, 0xf000000000000ull, 0x1f000000000000ull, 0x3f000000000000ull, 0x7f000000000000ull};

// Positive-slope diagonal bitmask table as above
const uint64_t PDIAG_BITMASK_TABLE[55] = {0x40201008040201ull, 0x402010080402ull, 0x4020100804ull, 0x40201008ull, 0x402010ull, 0x4020ull, 0x40ull, 0x0ull,
										  0x20100804020100ull, 0x40201008040201ull, 0x402010080402ull, 0x4020100804ull, 0x40201008ull, 0x402010ull, 0x4020ull, 0x0ull,
										  0x10080402010000ull, 0x20100804020100ull, 0x40201008040201ull, 0x402010080402ull, 0x4020100804ull, 0x40201008ull, 0x402010ull, 0x0ull,
										  0x8040201000000ull, 0x10080402010000ull, 0x20100804020100ull, 0x40201008040201ull, 0x402010080402ull, 0x4020100804ull, 0x40201008ull, 0x0ull,
										  0x4020100000000ull, 0x8040201000000ull, 0x10080402010000ull, 0x20100804020100ull, 0x40201008040201ull, 0x402010080402ull, 0x4020100804ull, 0x0ull,
										  0x2010000000000ull, 0x4020100000000ull, 0x8040201000000ull, 0x10080402010000ull, 0x20100804020100ull, 0x40201008040201ull, 0x402010080402ull, 0x0ull,
										  0x1000000000000ull, 0x2010000000000ull, 0x4020100000000ull, 0x8040201000000ull, 0x10080402010000ull, 0x20100804020100ull, 0x40201008040201ull};

// Negative-slope diagonals
const uint64_t NDIAG_BITMASK_TABLE[55] = {0x1ull, 0x102ull, 0x10204ull, 0x1020408ull, 0x102040810ull, 0x10204081020ull, 0x1020408102040ull, 0x0ull,
										 0x102ull, 0x10204ull, 0x1020408ull, 0x102040810ull, 0x10204081020ull, 0x1020408102040ull, 0x2040810204000ull, 0x0ull,
										 0x10204ull, 0x1020408ull, 0x102040810ull, 0x10204081020ull, 0x1020408102040ull, 0x2040810204000ull, 0x4081020400000ull, 0x0ull,
										 0x1020408ull, 0x102040810ull, 0x10204081020ull, 0x1020408102040ull, 0x2040810204000ull, 0x4081020400000ull, 0x8102040000000ull, 0x0ull,
										 0x102040810ull, 0x10204081020ull, 0x1020408102040ull, 0x2040810204000ull, 0x4081020400000ull, 0x8102040000000ull, 0x10204000000000ull, 0x0ull,
										 0x10204081020ull, 0x1020408102040ull, 0x2040810204000ull, 0x4081020400000ull, 0x8102040000000ull, 0x10204000000000ull, 0x20400000000000ull, 0x0ull,
										 0x1020408102040ull, 0x2040810204000ull, 0x4081020400000ull, 0x8102040000000ull, 0x10204000000000ull, 0x20400000000000ull, 0x40000000000000ull};

// The core Make Seven structure in which all movements and calculations are performed here.
typedef struct {
	uint64_t playerTiles[2], twoAndThreeTiles[2];			// The bitboard of each player's number tiles. One tiles are computed, while the twos and threes are stored.
	uint8_t remainingTiles[3];								// Remaining tiles left to play. The lower four bits are Green's, and the upper four bits are Yellow's.
	uint8_t height[MAKESEVEN_SIZE], plyNumber;				// Tile column height, and number of piles or half-moves.
	uint8_t movesHistory[MAKESEVEN_AREA];					// Variable to store the history of played moves in this round.
} MakeSeven;

// Helper functions
uint64_t intpow(uint64_t, uint64_t);						// Integer binary exponentation helper function.
int intlog2(uint64_t);										// Integer binary logarithm helper function.
int popcount(uint64_t);										// Count the number of bits in a 64-bit integer.

// Memory and I/O
void MakeSeven_initialize(MakeSeven*);						// Reset the Make Seven data structure to the initial position.
void MakeSeven_print(const MakeSeven*);						// Print the board of number tiles and their amount to the console.

// Logical functions
bool MakeSeven_tilesSumToSeven(const MakeSeven*);			// Returns true if the board configuration has an arrangement of tiles that sums exactly to seven.
bool MakeSeven_gameOver(const MakeSeven*);					// Test if the game is over, i.e. a player has a winning alignment or neither have any tiles left.
bool MakeSeven_hasNoMoreMoves(const MakeSeven*);			// True when the current player to move has no more legal moves and false otherwise.

// Move functions
bool MakeSeven_drop(MakeSeven*, const int, const int);		// Drop a number tile to a column if that column is not full.
void MakeSeven_undrop(MakeSeven*);							// Undo the dropping of number tiles--used only during search.

// User input functions
bool MakeSeven_getUserCharInput(MakeSeven*, const char);	// Make a move from user input. Numbers specify what tile to use and letters what column to drop.
bool MakeSeven_sequence(MakeSeven*, const char*);			// Make moves from a string of characters. This is used when the user chooses to pass them as arguments.

// Other functions
uint64_t MakeSeven_hashEncode(MakeSeven*);					// Encode a hashed Make Seven position for use in the transposition table.
uint64_t MakeSeven_reverse(uint64_t);						// Returns the reverse of a Make Seven grid bitboard.
bool MakeSeven_symmetrical(const MakeSeven*);				// Is the board's left side the same as its right side when flipped horizontally?

#endif
