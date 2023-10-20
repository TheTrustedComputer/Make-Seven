/*
    Copyright (C) 2020- TheTrustedComputer
*/

#include "make7.h"

void Make7_initialize(Make7* restrict _m7)
{
    int i;
    
    _m7->player[0] = _m7->player[1] = _m7->tiles23[0] = _m7->tiles23[1] = 0;
    _m7->remaining[0] = _m7->remaining[1] = 0xbb;
    _m7->remaining[2] = 0x44;
    _m7->plyNum = 0;
    g_inputReadyFlag = 0;
    
    for (i = 0; i < MAKE7_SIZE; i++)
    {
        _m7->height[i] = (uint8_t)(MAKE7_SIZE_P1 * i);
    }
}

void Make7_print(const Make7* restrict _M7)
{
    uint64_t bit, ONE_TILES = (_M7->player[0] | _M7->player[1]) ^ (_M7->tiles23[0] | _M7->tiles23[1]);
    uint8_t numOnes, numTwos, numThrees, turn = _M7->plyNum & 1;
    int i, j;
    
#if defined(_WIN64) || defined(_WIN32) // Get console handle
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
    
    if (turn) // Colorize coordinates on whose turn it is
    {
        
#if defined(_WIN64) || defined(_WIN32)
        SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
        printf("\e[1;93m");
#endif
        
    }
    else
    {
        
#if defined(_WIN64) || defined(_WIN32)
        SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
        printf("\e[1;92m");
#endif
        
    }
    
    // Print coordinates
    for (i = 0; i < MAKE7_SIZE; i++)
    {
        printf("%c ", i + 'A');
    }
    
#if defined(_WIN64) || defined(_WIN32)
    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    printf("\e[0m");
#endif
    puts("");
    
    // Print the playing grid
    for (i = MAKE7_SIZE; i--;)
    {
        for (j = 0; j < MAKE7_SIZE; j++)
        {
            // Compute the tile bit position
            bit = (1ull << (MAKE7_SIZE_P1 * j)) * (1ull << i);
            
            if (bit & _M7->player[0]) // Green's tiles
            {
                if (bit & ONE_TILES) // 1 tile
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, BACKGROUND_GREEN | BACKGROUND_INTENSITY);
                    printf("1");
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    printf(" ");
#else
                    printf("\e[1;7;92m1\e[0m ");
#endif
                    
                }
                else if (bit & _M7->tiles23[0]) // 2 tile
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, BACKGROUND_GREEN | BACKGROUND_INTENSITY);
                    printf("2");
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    printf(" ");
#else
                    printf("\e[1;7;92m2\e[0m ");
#endif
                    
                }
                else // 3 tile
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, BACKGROUND_GREEN | BACKGROUND_INTENSITY);
                    printf("3");
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    printf(" ");
#else
                    printf("\e[1;7;92m3\e[0m ");
#endif
                    
                }
            }
            else if (bit & _M7->player[1]) // Yellow's tiles
            {
                if (bit & ONE_TILES) // 1 tile
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY);
                    printf("1");
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    printf(" ");
#else
                    printf("\e[1;7;93m1\e[0m ");
#endif
                    
                }
                else if (bit & _M7->tiles23[0]) // 2 tile
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY);
                    printf("2");
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    printf(" ");
#else
                    printf("\e[1;7;93m2\e[0m ");
#endif
                    
                }
                else // 3 tile
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY);
                    printf("3");
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    printf(" ");
#else
                    printf("\e[1;7;93m3\e[0m ");
#endif
                    
                }
            }
            else 
            {
                if (bit & MAKE7_THREES) // Simulate red stickers on cells where 3 tiles are supposed to drop
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    printf("= ");
#else
                    printf("\e[91m= \e[0m");
#endif
                    
                }
                else
                {
                    
#if defined(_WIN64) || defined(_WIN32)
                    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    printf("- ");
#else
                    printf("\e[37;95m- \e[0m");
#endif
                    
                }
            }
        }
        
        if (i)
        {
            
#if defined(_WIN64) || defined(_WIN32)
            SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
            printf("\e[0m");
#endif
            
            puts("");
        }
    }
    
#if defined(_WIN64) || defined(_WIN32)
    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
    
    // Print remaining tiles
    numOnes = turn ? (_M7->remaining[0] >> 4) : (_M7->remaining[0] & 0xf);
    numTwos = turn ? (_M7->remaining[1] >> 4) : (_M7->remaining[1] & 0xf);
    numThrees = turn ? (_M7->remaining[2] >> 4) : (_M7->remaining[2] & 0xf);
    printf("\n1:%u 2:%u 3:%u\n", numOnes, numTwos, numThrees);
}

bool Make7_tilesSumTo7(const Make7* restrict _M7)
{
    uint64_t ALL_TWOS_BITMASK, ALL_THREES_BITMASK, PLAYER_TILES_BITMASK, PLAYER_ONES_BITMASK, PLAYER_TWOS_BITMASK, PLAYER_THREES_BITMASK, \
             CURR_ONES_BITMASK, CURR_TWOS_BITMASK, CURR_THREES_BITMASK, CURR_ALL_BITMASK, HORI_BOUND_BITMASK, tileBit;
    
    uint16_t adjacents;
    uint8_t MOVES_HISTORY, PLY_M1, NUMTILE_HEIGHT, winSum, shifter, bitsetter;
    
#ifndef NO_SLIDERS
    uint8_t winEnd;
#endif
    
    // Variables initialization
    PLY_M1 = _M7->plyNum - 1;
    MOVES_HISTORY = _M7->movesHist[PLY_M1];
    NUMTILE_HEIGHT = _M7->height[MOVES_HISTORY & 0xf] - 1;
    ALL_TWOS_BITMASK = _M7->tiles23[0];
    ALL_THREES_BITMASK = _M7->tiles23[1];
    PLAYER_TILES_BITMASK = _M7->player[PLY_M1 & 1];
    PLAYER_ONES_BITMASK = PLAYER_TILES_BITMASK ^ (PLAYER_TILES_BITMASK & (ALL_TWOS_BITMASK | ALL_THREES_BITMASK));
    PLAYER_TWOS_BITMASK = PLAYER_TILES_BITMASK & ALL_TWOS_BITMASK;
    PLAYER_THREES_BITMASK = PLAYER_TILES_BITMASK & ALL_THREES_BITMASK;
    HORI_BOUND_BITMASK = MAKE7_BOT << (NUMTILE_HEIGHT & MAKE7_SIZE);
    
    // Test for all four directions and set shift direction
    for (shifter = 0; shifter <= 3; shifter++)
    {
        
        // Set bit masks for potential wins per direction
        switch ((bitsetter = DIRECTION_TABLE[shifter]))
        {
        case 1:
            CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
            break;
        case MAKE7_SIZE_P1:
            CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & HORI_BOUND_BITMASK;
            CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & HORI_BOUND_BITMASK;
            CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & HORI_BOUND_BITMASK;
            CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & HORI_BOUND_BITMASK;
            break;
        case MAKE7_SIZE:
            CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            break;
        case MAKE7_SIZE_P2:
            CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
        }
        
        // (Re)set looping variables; move tileBit to the leftmost tile before hitting an empty tile
        for (adjacents = 0, tileBit = 1ull << NUMTILE_HEIGHT; CURR_ALL_BITMASK & (tileBit << bitsetter); tileBit <<= bitsetter);
        
        // Search tiles in this direction; it is guaranteed that there is at least one found adjacent tile
        // It is a 16-bit integer to store binary encoded adjacent tiles (2 bits per tile, 7 tiles max, totaling 14 bits)
        // 00: no tiles
        // 01: tile #1
        // 10: tile #2
        // 11: tile #3
        do
        {
            if (CURR_ONES_BITMASK & tileBit)
            {
                adjacents = (adjacents << 2) | 1; // Found tile #1s
            }
            else if (CURR_TWOS_BITMASK & tileBit)
            {
                adjacents = (adjacents << 2) | 2; // Found tile #2s
            }
            else if (CURR_THREES_BITMASK & tileBit)
            {
                adjacents = (adjacents << 2) | 3; // Found tile #3s
            }
            
            // Shift toward infinity to find the next tile
            CURR_ONES_BITMASK <<= bitsetter;
            CURR_TWOS_BITMASK <<= bitsetter;
            CURR_THREES_BITMASK <<= bitsetter;
        }
        while (CURR_ALL_BITMASK &= (CURR_ALL_BITMASK << bitsetter));
        
        // Minimum number of adjacent tiles is 3 since this is the smallest quantity of tiles that can add to seven
        // 0x30 is the bit mask for the third adjacent tile; same as the condition if (totalTiles >= 3)
        
#ifndef NO_SLIDERS
        if (adjacents & 0x30)
        {
#endif
            // Sum all found adjacent tiles and see if they "Make 7".
            // There are eight unique ways of adding to 7 given numbers 1, 2, and 3, equalling 44 combinations:
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
            // In other words, there can be a superset of the above eight ways of adding to 7.
            //
            // Below is a variant of the sliding window algorithm starting with a window size of 3 when the window sum is greater than 7.
            // This approach does not use arrays; thus, it reduces the memory access overhead, making this slightly more efficient.
            
#ifdef NO_SLIDERS // A more simpler but slower approach that doesn't use sliding windows, just sum and check. Compile with -DNO_SLIDERS to use this.
            winSum = (adjacents & 0x3) + ((adjacents & 0xc) >> 2) + ((adjacents & 0x30) >> 4);
#else
            winSum = (adjacents & 0x3) + ((adjacents & 0xc) >> 2) + ((adjacents & 0x30) >> (winEnd = 4));
#endif
            
#ifdef NO_SLIDERS
            while (adjacents & 0x30)
#else
            while (adjacents & (0x3 << winEnd))
#endif
            {
                if (winSum == 7) // The current player wins if they have a subset sum of 7
                {
                    return true;
                }
                
#ifdef NO_SLIDERS
                // Shift and add the next tile to check
                winSum += ((adjacents >>= 2) & 0x3);
#else
                
                if (winSum > 7) // Shift window to the right, subtracting the leftmost tile
                {
                    winSum -= (adjacents >>= 2) & 0x3;
                    winEnd -= 2;
                }
                else // Add running sum to the rightmost tile
                {
                    winEnd += 2;
                    winSum += (adjacents & (0x3 << winEnd)) >> winEnd;
                }
#endif
            }
        
#ifndef NO_SLIDERS
        }
#endif
    
    }
    
    return false;
}

bool Make7_gameOver(const Make7* restrict _M7)
{
    return _M7->plyNum && (Make7_tilesSumTo7(_M7) || Make7_noMoreMoves(_M7));
}

bool Make7_noMoreMoves(const Make7* restrict _M7)
{
    uint8_t rem1, rem2, rem3, TURN = _M7->plyNum & 1;
    
    // Get the number of remaining tiles for that player; loop is unrolled.
    rem1 = TURN ? (_M7->remaining[0] >> 4) : (_M7->remaining[0] & 0xf);
    rem2 = TURN ? (_M7->remaining[1] >> 4) : (_M7->remaining[1] & 0xf);
    rem3 = TURN ? (_M7->remaining[2] >> 4) : (_M7->remaining[2] & 0xf);
    
    // Does the current player has 1 and 2 tiles left?
    // Yes: Play it through until the latter condition from below is met.
    // No: Does the board state allows droppable 3 tiles? If so, the player to move is able to drop them in this case.
    return (rem1 || rem2) ? false : !(rem3 && ((_M7->player[0] | _M7->player[1]) + MAKE7_BOT) & MAKE7_THREES);
}

bool Make7_gridFull(const Make7* restrict _M7)
{
    return _M7->plyNum == MAKE7_AREA;
}

bool Make7_drop(Make7* restrict _m7, const uint8_t _NUM_TILE, const uint8_t _COLUMN)
{
    // Turn on a single bit that drops a number tile to that column
    uint64_t droppedTile = 1ull << _m7->height[_COLUMN];
    uint8_t _NUM_TILE_M1 = _NUM_TILE - 1, TURN = _m7->plyNum & 1, tileAmount = TURN ? (_m7->remaining[_NUM_TILE_M1] >> 4) : (_m7->remaining[_NUM_TILE_M1] & 0xf);
    
    // Is the column not full of tiles, and does the player have any tiles left?
    if (!(droppedTile & MAKE7_TOP) && tileAmount)
    {
        // If this tile is a 3 tile; are they dropped at their specified locations?
        if ((_NUM_TILE == 3) && !(droppedTile & MAKE7_THREES))
        {
            return false; // The 3 tiles are not dropped to where they're supposed to be
        }
        
        // This drop is legal. Now bitwise-or it with the bitmap of that player's dropped tiles
        _m7->player[TURN] |= droppedTile;
        
        // The Make7 structure does not have any means of saving one tiles; check if this tile is not a 1 and bitwise-or to the 2-and-3 tiles variable.
        // If no bit in _m7->tiles23 is flipped on and there is a bit in _m7->player at the same spot, then it is guaranteed to be a 1 tile.
        if (_NUM_TILE_M1)
        {
            _m7->tiles23[_NUM_TILE_M1 - 1] |= droppedTile;
        }
        
        // Store this move to the move variation history
        _m7->movesHist[_m7->plyNum++] = _COLUMN | (_NUM_TILE << 4);
        
        // Increase the column height where the tile was dropped in
        _m7->height[_COLUMN]++;
        
        // Deduct that number's remaining tiles from the given player
        _m7->remaining[_NUM_TILE_M1] = (TURN ? (_m7->remaining[_NUM_TILE_M1] & 0xf) : (_m7->remaining[_NUM_TILE_M1] & 0xf0)) | (--tileAmount << (TURN << 2));
        
        // Successful drop
        return true;
    }
    
    // Unsuccessful drop
    return false;
}

void Make7_undrop(Make7* restrict _m7)
{
    uint8_t lastDropHeight, lastNumTile, tileAmount, TURN;
    
    // Retrieve the last move from the movesHist array and also retrieve that tile
    lastDropHeight = _m7->movesHist[--_m7->plyNum] & 0xf;
    lastNumTile = (_m7->movesHist[_m7->plyNum] >> 4) - 1;
    TURN = _m7->plyNum & 1;
    tileAmount = TURN ? (_m7->remaining[lastNumTile] >> 4) : (_m7->remaining[lastNumTile] & 0xf);
        
    // Undo the bitwise-or operation by XORing with the last dropped tile
    _m7->player[TURN] ^= 1ull << --_m7->height[lastDropHeight];
    
    // Do not turn off this bit if the last tile played is a 1 tile
    if (lastNumTile)
    {
        _m7->tiles23[lastNumTile - 1] ^= 1ull << _m7->height[lastDropHeight];
    }
    
    // Reverse the drop move to the state before it occurred
    _m7->remaining[lastNumTile] = (TURN ? (_m7->remaining[lastNumTile] & 0xf) : (_m7->remaining[lastNumTile] & 0xf0)) | (++tileAmount << (TURN << 2));
}

bool Make7_getUserInput(Make7* restrict _m7, const char _INPUT)
{
    uint8_t number = _INPUT - '0';
    int8_t column = _INPUT - 'A', caseTest; // Uppercase
    
    if (!Make7_gameOver(_m7))
    {
        if (g_inputReadyFlag)
        {
            // Use case-insensitive input
            for (caseTest = 0; caseTest < 2; caseTest++)
            {
                if (column >= 0 && column < MAKE7_SIZE)
                {
                    g_inputReadyFlag = 0;
                    return Make7_drop(_m7, g_userNumberTile, column);
                }
                
                column = _INPUT - 'a'; // Lowercase
            }
        }
        else
        {
            // Check tile number input is in range
            if ((number > 0) && (number <= 3))
            {
                g_userNumberTile = number;
                g_inputReadyFlag = 1;
                
                return true;
            }
        }
    }
    
    return false;
}

bool Make7_sequence(Make7* restrict _m7, const char* restrict _SEQ)
{
    for (int s = 0; _SEQ[s]; s++)
    {
        if (!Make7_getUserInput(_m7, _SEQ[s]))
        {
            g_inputReadyFlag = 0;
            return false;
        }
    }
    
    return true;
}

uint64_t Make7_hashEncode(const Make7* restrict _M7)
{
    return _M7->player[_M7->plyNum & 1] + _M7->player[0] + _M7->player[1] + MAKE7_BOT;
}

uint64_t Make7_reverse(uint64_t _grid)
{
    uint64_t revGrid, revCols;
    
    for (revGrid = 0, revCols = MAKE7_SIZE_M1; _grid; _grid >>= MAKE7_SIZE_P1, revCols--)
    {
        revGrid |= (_grid & MAKE7_LCL) << (revCols * MAKE7_SIZE_P1);
    }
    
    return revGrid;
}

bool Make7_symmetrical(const Make7* restrict _M7)
{
    uint64_t revTiles[4] = {_M7->player[0], _M7->player[1], _M7->tiles23[0], _M7->tiles23[1]};
    
    // Reverse player tiles and number tiles
    revTiles[0] = Make7_reverse(revTiles[0]);
    revTiles[1] = Make7_reverse(revTiles[1]);
    revTiles[2] = Make7_reverse(revTiles[2]);
    revTiles[3] = Make7_reverse(revTiles[3]);
    
    // Test original and reversed to see if they are identical
    return (revTiles[0] == _M7->player[0]) && (revTiles[1] == _M7->player[1]) && (revTiles[2] == _M7->tiles23[0]) && (revTiles[3] == _M7->tiles23[1]);
}

void Make7_generate(const Make7* restrict _M7, uint8_t* restrict _list, uint8_t* restrict _count)
{
    uint64_t avail12Mask, avail3Mask, tileMask;
    uint8_t _1TilesLeft, _2TilesLeft, _3TilesLeft, column, TURN;
    
    TURN = _M7->plyNum & 1;
    avail12Mask = ((_M7->player[0] | _M7->player[1]) + MAKE7_BOT) & MAKE7_ALL;
    avail3Mask = avail12Mask & MAKE7_THREES;
    
    // Obtain tiles left for the current player
    _1TilesLeft = TURN ? (_M7->remaining[0] >> 4) : (_M7->remaining[0] & 0xf);
    _2TilesLeft = TURN ? (_M7->remaining[1] >> 4) : (_M7->remaining[1] & 0xf);
    _3TilesLeft = TURN ? (_M7->remaining[2] >> 4) : (_M7->remaining[2] & 0xf);
    *_count = 0;
    
    // Generate the list of available drops
    while (avail12Mask)
    {
        // Set a single bit to a droppable column
        tileMask = avail12Mask & -avail12Mask;
        column = __builtin_ctzll(tileMask) >> 3; // column / MAKE7_SIZE_P1
        
        if (_1TilesLeft)
        {
            _list[(*_count)++] = 0x10 | column; // (1 << 4)
        }
        
        if (_2TilesLeft)
        {
            _list[(*_count)++] = 0x20 | column; // (2 << 4)
        }
        
        if ((avail3Mask & tileMask) && _3TilesLeft)
        {
            _list[(*_count)++] = 0x30 | column; // (3 << 4)
        }
        
        // Clear the least significant set bit
        avail12Mask &= ~tileMask;
        avail3Mask &= ~tileMask;
    }
}

bool Make7_checkFor7(const Make7* restrict _M7)
{
    Make7 check7 = *_M7;
    uint64_t avail12Mask, avail3Mask, tileMask;
    uint8_t _1TilesLeft, _2TilesLeft, _3TilesLeft, column, TURN;
    
    TURN = check7.plyNum & 1;
    avail12Mask = ((check7.player[0] | check7.player[1]) + MAKE7_BOT) & MAKE7_ALL;
    avail3Mask = avail12Mask & MAKE7_THREES;
    
    // Obtain tiles left for the current player
    _1TilesLeft = TURN ? (check7.remaining[0] >> 4) : (check7.remaining[0] & 0xf);
    _2TilesLeft = TURN ? (check7.remaining[1] >> 4) : (check7.remaining[1] & 0xf);
    _3TilesLeft = TURN ? (check7.remaining[2] >> 4) : (check7.remaining[2] & 0xf);
    
    while (avail12Mask)
    {
        tileMask = avail12Mask & -avail12Mask;
        column = __builtin_ctzll(tileMask) >> 3;
        
        if (_1TilesLeft)
        {
            Make7_drop(&check7, 1, column);
            
            if (Make7_tilesSumTo7(&check7))
            {
                return true;
            }
            
            check7 = *_M7;
        }
        
        if (_2TilesLeft)
        {
            Make7_drop(&check7, 2, column);
            
            if (Make7_tilesSumTo7(&check7))
            {
                return true;
            }
            
            check7 = *_M7;
        }
        
        if ((avail3Mask & tileMask) && _3TilesLeft)
        {
            Make7_drop(&check7, 3, column);
            
            if (Make7_tilesSumTo7(&check7))
            {
                return true;
            }
            
            check7 = *_M7;
        }
        
        avail12Mask &= ~tileMask;
        avail3Mask &= ~tileMask;
    }
    
    return false;
}

void Make7_helpMessage(const char* restrict _NAME)
{
    printf("Usage: %s <switch> [ARGS]\n\n", _NAME);
    puts("Game solver for Make 7. It is a Connect Four variant where instead of colored");
    puts("discs, players drop tiles numbered 1 to 3 on a 7x7 grid. Tiles #1 and #2 can");
    puts("drop anywhere, but tile #3 can only drop in specific locations marked by red");
    puts("squares. The object is to be the first to align these tiles so that their sum");
    puts("equals 7. It is a draw if a player runs out of tiles to drop, or if the grid");
    puts("becomes full without a winner. The solver accepts the following command-line");
    puts("switches:\n");
    puts(" -g --profile-guided\tIgnores user input to generate a profile-guided");
    puts("\t\t\toptimization profile to improve performance. Note that");
    puts("\t\t\tthis requires a supported compiler.\n");
    puts(" -h -? --help\t\tDisplays this help message and exit.\n");
    puts(" -i --interactive\tAllows the user to play interactively. They can play");
    puts("\t\t\tagainst the Monte Carlo tree search AI or an additional");
    puts("\t\t\thuman player. It is impossible to play against the");
    puts("\t\t\tminimax AI at this time.\n");
    printf(" -m --mcts\t\tUses Monte Carlo tree search instead of minimax to solve");
    puts("\n\t\t\tthe game. Cancel anytime by hitting Ctrl+C.\n");
    puts(" -p --parallel\t\tParallelizes the search at the root position. This is");
    puts("\t\t\texperimental and may not work properly in every case.\n");
    puts(" -s --swap-colors\tSwaps the colors of the tiles. Instead of Green going");
    puts("\t\t\tfirst, Yellow will be going first.\n");
    printf(" -t --table-size [SIZE]\tModifies the transposition table entry size to [SIZE]");
    puts("\n\t\t\tgigabytes. Its absence will use all of the available");
    puts("\t\t\tmemory. If a zero or a negative value is given, it will");
    printf("\t\t\tallocate the smallest positive value possible. Note that");
    puts("\n\t\t\tthis switch is not applicable when using Monte Carlo");
    puts("\t\t\ttree search.\n");
    puts(" [SEQ1][SEQ2][SEQ3]...\tPlays a sequence of moves. The sequence must be in the");
    printf("\t\t\tfollowing format: [[1-3][A-G]] where the first character");
    puts("\n\t\t\tis the tile number, and the second character is the");
    puts("\t\t\tcolumn letter starting from the left. For example, the");
    puts("\t\t\tsequence 2D2C will drop tile #2 in column D, then");
    puts("\t\t\tanother one in column C. It will ignore any invalid");
    puts("\t\t\tmoves. The solver will exit after a solution is found,");
    puts("\t\t\tor if the game has ended in a win or a draw for one");
    puts("\t\t\tside. If using Monte Carlo tree search, it will exit");
    puts("\t\t\twhen the user presses Ctrl+C.\n");
    puts("Running the solver with no arguments will prompt the user to input a move");
    puts("sequence described above. Then, it will display the solution, the number of");
    puts("game positions evaluated, the speed of the evaluation, and the time spent");
    printf("solving that position in seconds. It will also find the best move by solving all");
    puts("\npossible moves from the current position and display it. These steps repeat");
    puts("until the user quits the solver in some way.");
}
