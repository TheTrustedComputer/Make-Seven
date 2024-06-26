/*
    Copyright (C) 2020- TheTrustedComputer
*/

#include "make7.h"

void Make7_initialize(Make7* restrict _m7)
{
    _m7->player[0] = _m7->player[1] = _m7->tiles23[0] = _m7->tiles23[1] = 0;
    _m7->remaining[0] = _m7->remaining[1] = 0xbb;
    _m7->remaining[2] = 0x44;
    _m7->turn = false;
    g_inputReadyFlag = 0;
    
    for (int i = 0; i < MAKE7_SIZE; i++)
    {
        _m7->height[i] = (uint8_t)(MAKE7_SIZE_P1 * i);
    }
}

void Make7_print(const Make7* restrict _M7)
{
    uint64_t ONE_TILES = (_M7->player[0] | _M7->player[1]) ^ (_M7->tiles23[0] | _M7->tiles23[1]);
    int i, j;
    
#if defined(_WIN64) || defined(_WIN32) // Get console handle
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
    
    if (_M7->turn) // Colorize coordinates on whose turn it is
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
            uint64_t bit = (1ull << (MAKE7_SIZE_P1 * j)) * (1ull << i);
            
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
    uint8_t numOnes = _M7->turn ? (_M7->remaining[0] >> 4) : (_M7->remaining[0] & 0xf);
    uint8_t numTwos = _M7->turn ? (_M7->remaining[1] >> 4) : (_M7->remaining[1] & 0xf);
    uint8_t numThrees = _M7->turn ? (_M7->remaining[2] >> 4) : (_M7->remaining[2] & 0xf);
    
    printf("\n1:%u 2:%u 3:%u\n", numOnes, numTwos, numThrees);
}

bool Make7_tilesSumTo7(const Make7* restrict _M7)
{
    uint64_t ALL_TWOS_BITMASK, ALL_THREES_BITMASK, PLAYER_TILES_BITMASK, PLAYER_ONES_BITMASK, PLAYER_TWOS_BITMASK, PLAYER_THREES_BITMASK, \
             CURR_ONES_BITMASK, CURR_TWOS_BITMASK, CURR_THREES_BITMASK, CURR_ALL_BITMASK, HORI_BOUND_BITMASK, VERT_CHECK, HORI_CHECK, \
             POS_DIAG_CHECK, NEG_DIAG_CHECK, PLAYER_ADJ_BITMASK, TILE_BIT;
    
    uint16_t adjacents;
    uint8_t NUMTILE_HEIGHT, winSum, dir, shifter;
    
#ifndef NO_SLIDERS
    uint8_t winEnd;
#endif
    
    // Variables initialization
    ALL_TWOS_BITMASK = _M7->tiles23[0];
    ALL_THREES_BITMASK = _M7->tiles23[1];
    PLAYER_TILES_BITMASK = _M7->player[!_M7->turn];
    PLAYER_ONES_BITMASK = PLAYER_TILES_BITMASK ^ (PLAYER_TILES_BITMASK & (ALL_TWOS_BITMASK | ALL_THREES_BITMASK));
    PLAYER_TWOS_BITMASK = PLAYER_TILES_BITMASK & ALL_TWOS_BITMASK;
    PLAYER_THREES_BITMASK = PLAYER_TILES_BITMASK & ALL_THREES_BITMASK;
    NUMTILE_HEIGHT = _M7->height[_M7->lastTile] - 1;
    HORI_BOUND_BITMASK = MAKE7_BOT << (NUMTILE_HEIGHT & MAKE7_SIZE);
    TILE_BIT = 1ull << NUMTILE_HEIGHT;
    PLAYER_ADJ_BITMASK = ADJ_BITMASK_TABLE[NUMTILE_HEIGHT] & PLAYER_TILES_BITMASK;
    VERT_CHECK = (TILE_BIT >> 1) & PLAYER_ADJ_BITMASK;
    HORI_CHECK = ((TILE_BIT >> MAKE7_SIZE_P1) | (TILE_BIT << MAKE7_SIZE_P1)) & PLAYER_ADJ_BITMASK;
    POS_DIAG_CHECK = ((TILE_BIT >> MAKE7_SIZE_P2) | (TILE_BIT << MAKE7_SIZE_P2)) & PLAYER_ADJ_BITMASK;
    NEG_DIAG_CHECK = ((TILE_BIT >> MAKE7_SIZE) | (TILE_BIT << MAKE7_SIZE)) & PLAYER_ADJ_BITMASK;
    
    // Test for all four directions and set shift direction
    for (dir = 0; dir <= 3; dir++)
    {
        // Set bit masks for potential wins per direction
        switch ((shifter = DIRECTION_TABLE[dir]))
        {
        case 1:
            if (VERT_CHECK) // Skip a direction if there are no potential tiles
            {
                CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
                CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
                CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
                CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
                break;
            }
            continue;
        case MAKE7_SIZE_P1:
            if (HORI_CHECK)
            {
                CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & HORI_BOUND_BITMASK;
                CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & HORI_BOUND_BITMASK;
                CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & HORI_BOUND_BITMASK;
                CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & HORI_BOUND_BITMASK;
                break;
            }
            continue;
        case MAKE7_SIZE:
            if (NEG_DIAG_CHECK)
            {
                CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
                CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
                CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
                CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
                break;
            }
            continue;
        case MAKE7_SIZE_P2:
            if (POS_DIAG_CHECK)
            {
                CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
                CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
                CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
                CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
                break;
            }
        default:
            continue;
        }
        
        // (Re)set looping variables; move TILE_BIT to the leftmost tile before hitting an empty cell
        for (adjacents = 0, TILE_BIT = 1ull << NUMTILE_HEIGHT; CURR_ALL_BITMASK & (TILE_BIT << shifter); TILE_BIT <<= shifter);
        
        // Search tiles in this direction; it is guaranteed that there is at least one found adjacent tile
        // It is a 16-bit integer to store binary encoded adjacent tiles (2 bits per tile, 7 tiles max, totaling 14 bits)
        // 00: blank
        // 01: #1 tiles
        // 10: #2 tiles
        // 11: #3 tiles
        do
        {
            if (CURR_ONES_BITMASK & TILE_BIT)
            {
                adjacents = (adjacents << 2) | 1; // Found #1 tiles
            }
            else if (CURR_TWOS_BITMASK & TILE_BIT)
            {
                adjacents = (adjacents << 2) | 2; // Found #2 tiles
            }
            else if (CURR_THREES_BITMASK & TILE_BIT)
            {
                adjacents = (adjacents << 2) | 3; // Found #3 tiles
            }
            
            // Shift toward infinity to find the next tile
            CURR_ONES_BITMASK <<= shifter;
            CURR_TWOS_BITMASK <<= shifter;
            CURR_THREES_BITMASK <<= shifter;
        }
        while (CURR_ALL_BITMASK &= (CURR_ALL_BITMASK << shifter));
        
        // Minimum number of adjacent tiles is 3 since this is the smallest quantity of tiles that can add to seven.
        // 0x30 is the bit mask for the third adjacent tile; same as the condition if (totalTiles >= 3).
        if (adjacents & 0x30)
        {
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
            // This approach does not use traditional arrays; thus, it reduces the memory access overhead, making this slightly more efficient.
            //
            // A more simpler but slower approach that doesn't use sliding windows, just sum and check. Compile with -DNO_SLIDERS to use this.  
#ifdef NO_SLIDERS
            winSum = (adjacents & 0x3) + ((adjacents & 0xc) >> 2) + ((adjacents & 0x30) >> 4);
            adjacents >>= 4; // Remove the three adjacent tiles since we already added them
#else
            winSum = (adjacents & 0x3) + ((adjacents & 0xc) >> 2) + ((adjacents & 0x30) >> (winEnd = 4));
#endif
            do
            {
#ifdef NO_SLIDERS
                if (!(adjacents & 0xc) && (winSum == 7)) // Search for exact sums of 7
                {
                    return true;
                }
                
                if (winSum >= 7) // Do not add the next tile if the sum is greater than or equal to 7   
                {
                    break;
                }
                
                // Shift and add the next tile to check
                winSum += ((adjacents >>= 2) & 0x3);             
#else               
                if (winSum == 7) // The current player wins if they have a subset (or exact) sum of 7
                {
                    return true;
                }

                if (winSum > 7) // Shift window to the right, subtracting the leftmost tile
                {
                    winSum -= (adjacents & 0x3);
                    adjacents >>= 2;
                    winEnd -= 2;
                }
                else // Add running sum to the rightmost tile
                {
                    winEnd += 2;
                    winSum += (adjacents & (0x3 << winEnd)) >> winEnd;
                }
#endif     
            }
#ifdef NO_SLIDERS
            while (adjacents);
#else
            while (adjacents & (0x3 << winEnd));
#endif    
        }
    }
    
    return false;
}

inline bool Make7_gameOver(const Make7* restrict _M7)
{
    return Make7_tilesSumTo7(_M7) || Make7_noMoreMoves(_M7);
}

bool Make7_noMoreMoves(const Make7* restrict _M7)
{
    // Get the number of remaining tiles for that player; loop is unrolled.
    uint8_t rem1 = _M7->turn ? (_M7->remaining[0] >> 4) : (_M7->remaining[0] & 0xf);
    uint8_t rem2 = _M7->turn ? (_M7->remaining[1] >> 4) : (_M7->remaining[1] & 0xf);
    uint8_t rem3 = _M7->turn ? (_M7->remaining[2] >> 4) : (_M7->remaining[2] & 0xf);
    
    // Does the current player has 1 and 2 tiles left?
    // Yes: Play it through until the latter condition from below is met.
    // No: Does the board state allows droppable 3 tiles? If so, the player to move is able to drop them in this case.
    return !(rem1 || rem2 || (rem3 && (((_M7->player[0] | _M7->player[1]) + MAKE7_BOT) & MAKE7_THREES)));
}

inline bool Make7_gridFull(const Make7* restrict _M7)
{
    return Make7_plyNum(_M7) == MAKE7_AREA;
}

inline uint8_t Make7_plyNum(const Make7* restrict _M7)
{
    
#if (defined(__MINGW32__) || defined(__MINGW64__))
    return __builtin_popcountll(_M7->player[0] | _M7->player[1]);
#else
    return stdc_count_ones(_M7->player[0] | _M7->player[1]);
#endif
    
}

bool Make7_drop(Make7* restrict _m7, const uint8_t _NUM_TILE, const uint8_t _COLUMN)
{
    // Turn on a single bit that drops a number tile to that column.
    uint64_t droppedTile = 1ull << _m7->height[_COLUMN];
    uint8_t NUM_TILE_M1 = _NUM_TILE - 1, tileAmount = _m7->turn ? (_m7->remaining[NUM_TILE_M1] >> 4) : (_m7->remaining[NUM_TILE_M1] & 0xf);
    
    // Is the column not full of tiles, and does the player have any tiles left?
    if (!(droppedTile & MAKE7_TOP) && tileAmount)
    {
        // If this tile is a 3 tile; are they dropped at their specified locations?
        if ((_NUM_TILE == 3) && !(droppedTile & MAKE7_THREES))
        {
            return false; // The 3 tiles are not dropped to where they're supposed to be.
        }
        
        // This drop is legal. Now bitwise-or it with the bitmap of that player's dropped tiles.
        _m7->player[_m7->turn] |= droppedTile;
        
        // The Make7 structure does not have any means of saving one tiles; check if this tile is not a 1 and bitwise-or to the 2-and-3 tiles variable.
        // If no bit in _m7->tiles23 is flipped on and there is a bit in _m7->player at the same spot, then it is guaranteed to be a 1 tile.
        if (NUM_TILE_M1)
        {
            _m7->tiles23[NUM_TILE_M1 - 1] |= droppedTile;
        }
        
        // Store the column index of the last dropped tile to check for win conditions.
        // Increase the column height where the tile was dropped in.
        _m7->height[(_m7->lastTile = _COLUMN)]++;
        
        // Deduct that number's remaining tiles from the given player.
        _m7->remaining[NUM_TILE_M1] = (_m7->turn ? (_m7->remaining[NUM_TILE_M1] & 0xf) : (_m7->remaining[NUM_TILE_M1] & 0xf0)) | (--tileAmount << (_m7->turn << 2));
        
        // Alternate turns.
        _m7->turn = !_m7->turn;
        
        return true;
    }
    
    return false;
}

/*
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
}*/

bool Make7_getUserInput(Make7* restrict _m7, const char _INPUT)
{
    uint8_t number = _INPUT - '0';
    int8_t column = _INPUT - 'A';
    
    if (!Make7_gameOver(_m7))
    {
        if (g_inputReadyFlag)
        {
            // Use case-insensitive input
            for (int8_t caseTest = 0; caseTest < 2; caseTest++) // Uppercase
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

inline uint64_t Make7_hashEncode(const Make7* restrict _M7)
{
    return _M7->player[_M7->turn] + _M7->player[0] + _M7->player[1] + MAKE7_BOT;
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
    uint64_t avail12Mask = ((_M7->player[0] | _M7->player[1]) + MAKE7_BOT) & MAKE7_ALL;
    uint64_t avail3Mask = avail12Mask & MAKE7_THREES;
    
    // Obtain tiles left for the current player
    uint8_t _1TilesLeft = _M7->turn ? (_M7->remaining[0] >> 4) : (_M7->remaining[0] & 0xf);
    uint8_t _2TilesLeft = _M7->turn ? (_M7->remaining[1] >> 4) : (_M7->remaining[1] & 0xf);
    uint8_t _3TilesLeft = _M7->turn ? (_M7->remaining[2] >> 4) : (_M7->remaining[2] & 0xf);
    
    *_count = 0;
    
    // Generate the list of available drops
    while (avail12Mask)
    {
        // Set a single bit to a droppable column
        uint64_t tileMask = avail12Mask & -avail12Mask;
        
#if (defined(__MINGW32__) || defined(__MINGW64__))
        uint8_t column = __builtin_ctzll(tileMask) >> 3;
#else
        uint8_t column = stdc_trailing_zeros(tileMask) >> 3; // column / MAKE7_SIZE_P1
#endif
        
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
    uint64_t avail12Mask = ((_M7->player[0] | _M7->player[1]) + MAKE7_BOT) & MAKE7_ALL;
    uint64_t avail3Mask = avail12Mask & MAKE7_THREES;
    
    uint8_t _1TilesLeft = _M7->turn ? (_M7->remaining[0] >> 4) : (_M7->remaining[0] & 0xf);
    uint8_t _2TilesLeft = _M7->turn ? (_M7->remaining[1] >> 4) : (_M7->remaining[1] & 0xf);
    uint8_t _3TilesLeft = _M7->turn ? (_M7->remaining[2] >> 4) : (_M7->remaining[2] & 0xf);
    
    Make7 checkM7 = *_M7;
    
    while (avail12Mask)
    {
        uint64_t tileMask = avail12Mask & -avail12Mask;
        
#if (defined(__MINGW32__) || defined(__MINGW64__))
        uint8_t column = __builtin_ctzll(tileMask) >> 3;
#else
        uint8_t column = stdc_trailing_zeros(tileMask) >> 3;
#endif
        
        if (_1TilesLeft)
        {
            Make7_drop(&checkM7, 1, column);
            
            if (Make7_tilesSumTo7(&checkM7))
            {
                return true;
            }
            
            checkM7 = *_M7;
        }
        
        if (_2TilesLeft)
        {
            Make7_drop(&checkM7, 2, column);
            
            if (Make7_tilesSumTo7(&checkM7))
            {
                return true;
            }
            
            checkM7 = *_M7;
        }
        
        if ((avail3Mask & tileMask) && _3TilesLeft)
        {
            Make7_drop(&checkM7, 3, column);
            
            if (Make7_tilesSumTo7(&checkM7))
            {
                return true;
            }
            
            checkM7 = *_M7;
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
    puts("\t\t\toptimization file to improve performance. Note that this");
    puts("\t\t\trequires a supported compiler.\n");
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
