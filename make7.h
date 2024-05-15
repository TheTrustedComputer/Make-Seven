/*
    Copyright (C) 2020- TheTrustedComputer
    
    This is a computer implementation of Make 7, a Connect 4-like game involving elementary additon.
    The objective of this game is to drop numbers that add up to seven in a vertical, horizontal, or diagonal matter.
    
    The physical game ships 11 one tiles, 11 two tiles, and 4 three tiles per player, totaling 52 tiles.
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
#ifndef MAKE7_H
#define MAKE7_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Make 7's board dimensions is a fixed seven-by-seven square to take into account of the one tiles.
#define MAKE7_SIZE 7
#define MAKE7_SIZE_M1 6
#define MAKE7_SIZE_P1 8
#define MAKE7_SIZE_P2 9
#define MAKE7_SIZE_X3 21
#define MAKE7_AREA 49
#define MAKE7_AREA_P1 50
#define MAKE7_AREA_X2 98

// Make 7 static bitmaps; it works pretty much more or less the same as any Connect Four implementation.
#define MAKE7_ALL 0x7f7f7f7f7f7f7full
#define MAKE7_BOT 0x1010101010101ull
#define MAKE7_TOP 0x80808080808080ull
#define MAKE7_LCL 0x7full

// In the physical game, the three tiles can only be dropped at the marked red squares; ones and twos can drop anywhere.
#define MAKE7_THREES 0x4102008201004ull

// The players are named Green and Yellow after the included colored tiles for convenience.
#define MAKE7_P1_NAME "Green"
#define MAKE7_P2_NAME "Yellow"

// Global temporary storage locations or buffers for the number tiles from user input
static uint8_t g_userNumberTile, g_inputReadyFlag;

// Whether to swap the tile colors in the output
static bool g_swapColors;

// Vertical bitmask table to search for vertical connections below this tile, including itself
static const uint64_t VERT_BITMASK_TABLE[55] = {0x1ull, 0x3ull, 0x7ull, 0xfull, 0x1full, 0x3full, 0x7full, 0x0ull,
                                                0x100ull, 0x300ull, 0x700ull, 0xf00ull, 0x1f00ull, 0x3f00ull, 0x7f00ull, 0x0ull,
                                                0x10000ull, 0x30000ull, 0x70000ull, 0xf0000ull, 0x1f0000ull, 0x3f0000ull, 0x7f0000ull, 0x0ull,
                                                0x1000000ull, 0x3000000ull, 0x7000000ull, 0xf000000ull, 0x1f000000ull, 0x3f000000ull, 0x7f000000ull, 0x0ull,
                                                0x100000000ull, 0x300000000ull, 0x700000000ull, 0xf00000000ull, 0x1f00000000ull, 0x3f00000000ull, 0x7f00000000ull, 0x0ull,
                                                0x10000000000ull, 0x30000000000ull, 0x70000000000ull, 0xf0000000000ull, 0x1f0000000000ull, 0x3f0000000000ull, 0x7f0000000000ull, 0x0ull,
                                                0x1000000000000ull, 0x3000000000000ull, 0x7000000000000ull, 0xf000000000000ull, 0x1f000000000000ull, 0x3f000000000000ull, 0x7f000000000000ull};

// Positive-slope diagonal bitmask table as above
static const uint64_t PDIAG_BITMASK_TABLE[55] = {0x40201008040201ull, 0x402010080402ull, 0x4020100804ull, 0x40201008ull, 0x402010ull, 0x4020ull, 0x40ull, 0x0ull,
                                                 0x20100804020100ull, 0x40201008040201ull, 0x402010080402ull, 0x4020100804ull, 0x40201008ull, 0x402010ull, 0x4020ull, 0x0ull,
                                                 0x10080402010000ull, 0x20100804020100ull, 0x40201008040201ull, 0x402010080402ull, 0x4020100804ull, 0x40201008ull, 0x402010ull, 0x0ull,
                                                 0x8040201000000ull, 0x10080402010000ull, 0x20100804020100ull, 0x40201008040201ull, 0x402010080402ull, 0x4020100804ull, 0x40201008ull, 0x0ull,
                                                 0x4020100000000ull, 0x8040201000000ull, 0x10080402010000ull, 0x20100804020100ull, 0x40201008040201ull, 0x402010080402ull, 0x4020100804ull, 0x0ull,
                                                 0x2010000000000ull, 0x4020100000000ull, 0x8040201000000ull, 0x10080402010000ull, 0x20100804020100ull, 0x40201008040201ull, 0x402010080402ull, 0x0ull,
                                                 0x1000000000000ull, 0x2010000000000ull, 0x4020100000000ull, 0x8040201000000ull, 0x10080402010000ull, 0x20100804020100ull, 0x40201008040201ull};

// Negative-slope diagonals
static const uint64_t NDIAG_BITMASK_TABLE[55] = {0x1ull, 0x102ull, 0x10204ull, 0x1020408ull, 0x102040810ull, 0x10204081020ull, 0x1020408102040ull, 0x0ull,
                                                 0x102ull, 0x10204ull, 0x1020408ull, 0x102040810ull, 0x10204081020ull, 0x1020408102040ull, 0x2040810204000ull, 0x0ull,
                                                 0x10204ull, 0x1020408ull, 0x102040810ull, 0x10204081020ull, 0x1020408102040ull, 0x2040810204000ull, 0x4081020400000ull, 0x0ull,
                                                 0x1020408ull, 0x102040810ull, 0x10204081020ull, 0x1020408102040ull, 0x2040810204000ull, 0x4081020400000ull, 0x8102040000000ull, 0x0ull,
                                                 0x102040810ull, 0x10204081020ull, 0x1020408102040ull, 0x2040810204000ull, 0x4081020400000ull, 0x8102040000000ull, 0x10204000000000ull, 0x0ull,
                                                 0x10204081020ull, 0x1020408102040ull, 0x2040810204000ull, 0x4081020400000ull, 0x8102040000000ull, 0x10204000000000ull, 0x20400000000000ull, 0x0ull,
                                                 0x1020408102040ull, 0x2040810204000ull, 0x4081020400000ull, 0x8102040000000ull, 0x10204000000000ull, 0x20400000000000ull, 0x40000000000000ull};

// Bitmask of adjacent tiles; below is a sample illustration:
// 1 1 1
// 1 0 1
// 1 1 1
static const uint64_t ADJ_BITMASK_TABLE[55] = {0x302ll, 0x705ll, 0xe0all, 0x1c14ll, 0x3828ll, 0x7050ll, 0x6020ll, 0x0ll,
                                               0x30203ll, 0x70507ll, 0xe0a0ell, 0x1c141cll, 0x382838ll, 0x705070ll, 0x602060ll, 0x0ll,
                                               0x3020300ll, 0x7050700ll, 0xe0a0e00ll, 0x1c141c00ll, 0x38283800ll, 0x70507000ll, 0x60206000ll, 0x0ll,
                                               0x302030000ll, 0x705070000ll, 0xe0a0e0000ll, 0x1c141c0000ll, 0x3828380000ll, 0x7050700000ll, 0x6020600000ll, 0x0ll,
                                               0x30203000000ll, 0x70507000000ll, 0xe0a0e000000ll, 0x1c141c000000ll, 0x382838000000ll, 0x705070000000ll, 0x602060000000ll, 0x0ll,
                                               0x3020300000000ll, 0x7050700000000ll, 0xe0a0e00000000ll, 0x1c141c00000000ll, 0x38283800000000ll, 0x70507000000000ll, 0x60206000000000ll, 0x0ll,
                                               0x302030000000000ll, 0x705070000000000ll, 0xe0a0e0000000000ll, 0x1c141c0000000000ll, 0x3828380000000000ll, 0x7050700000000000ll, 0x6020600000000000ll};

// Direction tables to check for adjacent tiles
static const uint8_t DIRECTION_TABLE[4] = {1, MAKE7_SIZE_P1, MAKE7_SIZE, MAKE7_SIZE_P2};

// The core Make 7 structure in which all movements and calculations are performed here.
typedef struct
{
    uint64_t player[2], tiles23[2];                     // The bitboard of each player's number tiles and all dropped tiles except 1s.
    uint8_t movesHist[MAKE7_AREA];                      // Variable to store the history of played moves in this round.
    uint8_t height[MAKE7_SIZE];                         // The bit position of the height of each column of the grid.
    uint8_t remaining[3], plyNum;                       // Remaining tiles and the number of piles or half-moves.
}
Make7;

// Memory and I/O
void Make7_initialize(Make7*);                          // Resets the Make 7 data structure to the initial position.
void Make7_print(const Make7*);                         // Prints the board of number tiles and their amount to the console.

// Logical functions
bool Make7_tilesSumTo7(const Make7*);                   // Returns true if the board configuration has an arrangement of tiles that sums to at least seven.
bool Make7_gameOver(const Make7*);                      // Tests if the game is over, i.e. a player has a winning alignment or neither have any tiles left.
bool Make7_noMoreMoves(const Make7*);                   // True when the current player to move has no more legal moves and false otherwise.
bool Make7_gridFull(const Make7*);                      // Another draw condition is when the grid becomes full if either player has tiles left.

// Move functions
bool Make7_drop(Make7*, const uint8_t, const uint8_t);  // Drops a number tile to a column as long as that column is not full.
void Make7_undrop(Make7*);                              // Undos the dropping of number tiles--used only during search.

// User input functions
bool Make7_getUserInput(Make7*, const char);            // Makes a move from user input. Numbers specify what tile to use and letters what column to drop.
bool Make7_sequence(Make7*, const char*);               // Makes moves from a string of characters. This is used when the user chooses to pass them as arguments.

// Other functions
uint64_t Make7_hashEncode(const Make7*);                // Encodes a hashed Make 7 position for use in the transposition table.
uint64_t Make7_reverse(uint64_t);                       // Returns the horizontal inversion of a Make 7 grid bitboard.
bool Make7_symmetrical(const Make7*);                   // Is the board's left side the same as its right side when flipped horizontally?
void Make7_generate(const Make7*, uint8_t*, uint8_t*);  // Generates all possible drop moves for the current player.
void Make7_helpMessage(const char*);                    // Prints a help message to the console.

#endif /* MAKE7_H */
