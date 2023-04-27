/*
    Copyright (C) 2020- TheTrustedComputer
*/

#include "make7.h"

uint8_t MakeSeven_bitCount(uint64_t _b)
{
    uint8_t count = 0;
    
    while (_b)
    {
        ++count;
        _b &= _b - 1ull;
    }
    
    return count;
}

void MakeSeven_initialize(MakeSeven *_ms)
{
    int i;
    
    _ms->player[0] = _ms->player[1] = _ms->tiles23[0] = _ms->tiles23[1] = 0ull;
    _ms->remaining[0] = _ms->remaining[1] = 0xbb;
    _ms->remaining[2] = 0x44;
    _ms->plyNum = 0;
    inputReadyFlag = 0;
    
    for (i = 0; i < MAKESEVEN_SIZE; ++i)
    {
        _ms->height[i] = (uint8_t)(MAKESEVEN_SIZE_P1 * i);
    }
}

void MakeSeven_print(const MakeSeven *_MS)
{
    uint64_t bit, ONE_TILES = (_MS->player[0] | _MS->player[1]) ^ (_MS->tiles23[0] | _MS->tiles23[1]);
    uint8_t numOnes, numTwos, numThrees, turn = _MS->plyNum & 1;
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
    for (i = 0; i < MAKESEVEN_SIZE; ++i)
    {
        printf("%c ", i + 'A');
    }
    
#if defined(_WIN64) || defined(_WIN32)
    SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    printf("\e[0m");
#endif
    puts("");
    
    // Print playing grid
    for (i = MAKESEVEN_SIZE - 1; i >= 0; --i)
    {
        for (j = 0; j < MAKESEVEN_SIZE; ++j)
        {
            // Compute tile bit position
            bit = (1ull << (MAKESEVEN_SIZE_P1 * j)) * (1ull << i);
            
            if (bit & _MS->player[0]) // Green's tiles
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
                else if (bit & _MS->tiles23[0]) // 2 tile
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
            else if (bit & _MS->player[1]) // Yellow's tiles
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
                else if (bit & _MS->tiles23[0]) // 2 tile
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
                if (bit & MAKESEVEN_THREES) // Simulate red stickers on cells where 3 tiles are supposed to drop
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
                    printf("\e[95m- \e[0m");
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
    numOnes = turn ? (_MS->remaining[0] >> 4) : (_MS->remaining[0] & 0xf);
    numTwos = turn ? (_MS->remaining[1] >> 4) : (_MS->remaining[1] & 0xf);
    numThrees = turn ? (_MS->remaining[2] >> 4) : (_MS->remaining[2] & 0xf);
    printf("\n1:%u 2:%u 3:%u\n", numOnes, numTwos, numThrees);
}

bool MakeSeven_tilesSumToSeven(const MakeSeven *_MS)
{
    uint64_t ALL_TWOS_BITMASK, ALL_THREES_BITMASK, PLAYER_TILES_BITMASK, PLAYER_ONES_BITMASK, PLAYER_TWOS_BITMASK, PLAYER_THREES_BITMASK, \
             CURR_ONES_BITMASK, CURR_TWOS_BITMASK, CURR_THREES_BITMASK, CURR_ALL_BITMASK, HORI_BOUND_BITMASK, tileBit;
    uint16_t adjacents;
    uint8_t MOVES_HISTORY, PLY_M1, NUMTILE_HEIGHT, winSum, winEnd, shifter, bitsetter;
    
    // Variables initialization
    PLY_M1 = _MS->plyNum - 1;
    MOVES_HISTORY = _MS->movesHist[PLY_M1];
    NUMTILE_HEIGHT = _MS->height[MOVES_HISTORY & 0xf] - 1;
    ALL_TWOS_BITMASK = _MS->tiles23[0];
    ALL_THREES_BITMASK = _MS->tiles23[1];
    PLAYER_TILES_BITMASK = _MS->player[PLY_M1 & 1];
    PLAYER_ONES_BITMASK = PLAYER_TILES_BITMASK ^ (PLAYER_TILES_BITMASK & (ALL_TWOS_BITMASK | ALL_THREES_BITMASK));
    PLAYER_TWOS_BITMASK = PLAYER_TILES_BITMASK & ALL_TWOS_BITMASK;
    PLAYER_THREES_BITMASK = PLAYER_TILES_BITMASK & ALL_THREES_BITMASK;
    HORI_BOUND_BITMASK = MAKESEVEN_BOT << (NUMTILE_HEIGHT & MAKESEVEN_SIZE);
    
    // Test all four directions
    for (shifter = 0; shifter < 4; ++shifter)
    {
        // Set shift direction
        switch (shifter)
        {
        case 0:
            bitsetter = 1; // Vertical shift (|)
            break;
        case 1:
            bitsetter = MAKESEVEN_SIZE_P1; // Horizontal shift (-)
            break;
        case 2:
            bitsetter = MAKESEVEN_SIZE; // Negative sloped diagonal shift (\)
            break;
        case 3:
            bitsetter = MAKESEVEN_SIZE_P2; // Positive sloped diagonal shift (/)
        }
        
        // Set bitmasks for potential wins per direction
        switch (bitsetter)
        {
        case 1:
            CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & VERT_BITMASK_TABLE[NUMTILE_HEIGHT];
            break;
        case MAKESEVEN_SIZE_P1:
            CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & HORI_BOUND_BITMASK;
            CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & HORI_BOUND_BITMASK;
            CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & HORI_BOUND_BITMASK;
            CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & HORI_BOUND_BITMASK;
            break;
        case MAKESEVEN_SIZE:
            CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & NDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            break;
        case MAKESEVEN_SIZE_P2:
            CURR_ONES_BITMASK = PLAYER_ONES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_TWOS_BITMASK = PLAYER_TWOS_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_THREES_BITMASK = PLAYER_THREES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
            CURR_ALL_BITMASK = PLAYER_TILES_BITMASK & PDIAG_BITMASK_TABLE[NUMTILE_HEIGHT];
        }
        
        // (Re)set looping variables; move tileBit to the leftmost tile before hitting an empty tile
        for (adjacents = 0, tileBit = 1ull << NUMTILE_HEIGHT; CURR_ALL_BITMASK & (tileBit >> bitsetter); tileBit >>= bitsetter);
        
        // Search tiles in this direction and store them in the adjacents variable
        // It is a 16-bit integer to store binary encoded adjacent tiles (2 bits per tile, 7 tiles max, totaling 14 bits)
        // 00: no tiles
        // 01: tile #1
        // 10: tile #2
        // 11: tile #3
        while (CURR_ALL_BITMASK)
        {
            if (CURR_ONES_BITMASK & tileBit)
            {
                adjacents = (adjacents << 2) | 1; // Found one tiles
            }
            else if (CURR_TWOS_BITMASK & tileBit)
            {
                adjacents = (adjacents << 2) | 2; // Found two tiles
            }
            else if (CURR_THREES_BITMASK & tileBit)
            {
                adjacents = (adjacents << 2) | 3; // Found three tiles
            }
            
            // Shift toward zero to find the next tile
            CURR_ONES_BITMASK >>= bitsetter;
            CURR_TWOS_BITMASK >>= bitsetter;
            CURR_THREES_BITMASK >>= bitsetter;
            CURR_ALL_BITMASK &= (CURR_ALL_BITMASK >> bitsetter);
        }
        
        // Minimum number of adjacent tiles is 3 since this is the smallest quantity of tiles that can add to seven
        // 0x30 is the bit mask for the third adjacent tile; same as the condition (totalTiles >= 3)
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
            // This approach does not use arrays; thus, it reduces the memory access overhead, making this slightly more efficient.
            winSum = (adjacents & 0x3) + ((adjacents & 0xc) >> 2) + ((adjacents & 0x30) >> (winEnd = 4));
            
            while (adjacents & (0x3 << winEnd))
            {
                if (winSum == 7) // The current player wins if they have a subset sum of 7
                {
                    return true;
                }
                
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
            }
        }
    }
    
    return false;
}

bool MakeSeven_gameOver(const MakeSeven *_MS)
{
    return _MS->plyNum && (MakeSeven_tilesSumToSeven(_MS) || MakeSeven_hasNoMoreMoves(_MS));
}

bool MakeSeven_hasNoMoreMoves(const MakeSeven *_MS)
{
    uint8_t rem1, rem2, rem3, TURN = _MS->plyNum & 1;
    
    // Get the number of remaining tiles for that player; loop is unrolled.
    rem1 = TURN ? (_MS->remaining[0] >> 4) : (_MS->remaining[0] & 0xf);
    rem2 = TURN ? (_MS->remaining[1] >> 4) : (_MS->remaining[1] & 0xf);
    rem3 = TURN ? (_MS->remaining[2] >> 4) : (_MS->remaining[2] & 0xf);
    
    // Does the current player has 1 and 2 tiles left?
    // Yes: Play it through until the latter condition from below is met.
    // No: Does the board state allows droppable 3 tiles? If so, the player to move is able to drop them in this case.
    return (rem1 || rem2) ? false : !(rem3 && ((_MS->player[0] | _MS->player[1]) + MAKESEVEN_BOT) & MAKESEVEN_THREES);
}

bool MakeSeven_gridFull(const MakeSeven *_MS)
{
    return _MS->plyNum == MAKESEVEN_AREA;
}

bool MakeSeven_drop(MakeSeven *_ms, const uint8_t _NUM_TILE, const uint8_t _COLUMN)
{
    // Turn on a single bit that drops a number tile to that column
    uint64_t droppedTile = 1ull << _ms->height[_COLUMN];
    uint8_t _NUM_TILE_M1 = _NUM_TILE - 1, TURN = _ms->plyNum & 1, tileAmount = TURN ? (_ms->remaining[_NUM_TILE_M1] >> 4) : (_ms->remaining[_NUM_TILE_M1] & 0xf);
    
    // Is the column not full of tiles, and does the player have any tiles left?
    if (!(droppedTile & MAKESEVEN_TOP) && tileAmount)
    {
        if (_NUM_TILE == 3)
        {
            // If this tile is a 3 tile; are the 3 tiles dropped at their specified locations?
            if (!(droppedTile & MAKESEVEN_THREES))
            {
                return false; // The 3 tiles are not dropped to where they're supposed to be
            }
        }
        
        // This drop is legal. Now bitwise-or it with the bitmap of that player's dropped tiles
        _ms->player[TURN] |= droppedTile;
        
        // The MakeSeven structure does not have any means of saving one tiles in memory; check if this tile is not a 1 and bitwise-or to the two-and-three tiles variable.
        // If no bit in twoAndThreeTiles is flipped on and there is a bit in playerTiles at the same spot, then it is guaranteed to be a 1 tile.
        if (_NUM_TILE_M1)
        {
            _ms->tiles23[_NUM_TILE_M1 - 1] |= droppedTile;
        }
        
        // Store this move to the move variation history
        _ms->movesHist[_ms->plyNum++] = _COLUMN | (_NUM_TILE << 4);
        
        // Increase the column height where the tile was dropped in
        ++_ms->height[_COLUMN];
        
        // Deduct that number's remaining tiles from the given player
        _ms->remaining[_NUM_TILE_M1] = (TURN ? (_ms->remaining[_NUM_TILE_M1] & 0xf) : (_ms->remaining[_NUM_TILE_M1] & 0xf0)) | (--tileAmount << (TURN << 2));
        
        // Successful drop
        return true;
    }
    
    // Illegal drop
    return false;
}

void MakeSeven_undrop(MakeSeven *_ms)
{
    // Retrieve the last move from the movesHist array and also retrieve that tile
    uint8_t lastDroppedHeight = _ms->movesHist[--_ms->plyNum] & 0xf, TURN = _ms->plyNum & 1, lastNumTile = (_ms->movesHist[_ms->plyNum] >> 4) - 1, tileAmount;
        
    // Undo the bitwise-or operation by XORing with the last dropped tile
    _ms->player[TURN] ^= 1ull << --_ms->height[lastDroppedHeight];
    
    // Do not turn off this bit if the last tile played is a 1 tile
    if (lastNumTile)
    {
        _ms->tiles23[lastNumTile - 1] ^= 1ull << _ms->height[lastDroppedHeight];
    }
    
    // Reverse the drop move to the state before it occurred
    tileAmount = TURN ? (_ms->remaining[lastNumTile] >> 4) : (_ms->remaining[lastNumTile] & 0xf);
    _ms->remaining[lastNumTile] = (TURN ? (_ms->remaining[lastNumTile] & 0xf) : (_ms->remaining[lastNumTile] & 0xf0)) | (++tileAmount << (TURN << 2));
}

bool MakeSeven_getUserCharInput(MakeSeven *_ms, const char _INPUT)
{
    uint8_t number = _INPUT - '0';
    int8_t column = _INPUT - 'A', caseTest; // Uppercase
    
    if (!MakeSeven_gameOver(_ms))
    {
        if (inputReadyFlag)
        {
            // Use case-insensitive input
            for (caseTest = 0; caseTest < 2; ++caseTest)
            {
                if (column >= 0 && column < MAKESEVEN_SIZE)
                {
                    inputReadyFlag = 0;
                    return MakeSeven_drop(_ms, userNumberTile, column);
                }
                
                column = _INPUT - 'a'; // Lowercase
            }
        }
        else
        {
            // Check tile number input is in range
            if ((number > 0) && (number <= 3))
            {
                userNumberTile = number;
                inputReadyFlag = 1;
                
                return true;
            }
        }
    }
    
    return false;
}

bool MakeSeven_sequence(MakeSeven *_ms, const char *_SEQ)
{
    for (int s = 0; _SEQ[s]; ++s)
    {
        if (!MakeSeven_getUserCharInput(_ms, _SEQ[s]))
        {
            inputReadyFlag = 0;
            return false;
        }
    }
    
    return true;
}

uint64_t MakeSeven_hashEncode(const MakeSeven *_MS)
{
    return _MS->player[_MS->plyNum & 1] + _MS->player[0] + _MS->player[1] + MAKESEVEN_BOT;
}

uint64_t MakeSeven_reverse(uint64_t _grid)
{
    uint64_t revGrid, revCols;
    
    for (revGrid = 0ull, revCols = MAKESEVEN_SIZE_M1; _grid; _grid >>= MAKESEVEN_SIZE_P1, --revCols)
    {
        revGrid |= (_grid & MAKESEVEN_LCL) << (revCols * MAKESEVEN_SIZE_P1);
    }
    
    return revGrid;
}

bool MakeSeven_symmetrical(const MakeSeven *_MS)
{
    uint64_t revTiles[4] = { _MS->player[0], _MS->player[1], _MS->tiles23[0], _MS->tiles23[1] }, r;
    
    // Reverse player tiles and number tiles
    for (r = 0; r < 4; ++r)
    {
        revTiles[r] = MakeSeven_reverse(revTiles[r]);
    }
    
    // Test original and reversed to see if they are identical
    return (revTiles[0] == _MS->player[0]) && (revTiles[1] == _MS->player[1]) && (revTiles[2] == _MS->tiles23[0]) && (revTiles[3] == _MS->tiles23[1]);
}

void MakeSeven_generate(const MakeSeven *_MS, uint8_t *_list, uint8_t *_countPtr)
{
    uint64_t gridMask, avail12Mask, avail3Mask;
    uint8_t _1TilesLeft, _2TilesLeft, _3TilesLeft, foundIndex, column, popcnt12, TURN;
    
    TURN = _MS->plyNum & 1;
    gridMask = _MS->player[0] | _MS->player[1];
    avail12Mask = (gridMask + MAKESEVEN_BOT) & MAKESEVEN_ALL;
    avail3Mask = avail12Mask & MAKESEVEN_THREES;
    foundIndex = column = *_countPtr = 0;
    
#if defined (__GNUC__) || defined (__clang__) // Built-in compiler function
    popcnt12 = __builtin_popcountll(avail12Mask);
#else // Other compilers use an explicit function
    popcnt12 = MakeSeven_bitCount(avail12Mask);
#endif
    
    // Obtain tiles left for the current player
    _1TilesLeft = TURN ? (_MS->remaining[0] >> 4) : (_MS->remaining[0] & 0xf);
    _2TilesLeft = TURN ? (_MS->remaining[1] >> 4) : (_MS->remaining[1] & 0xf);
    _3TilesLeft = TURN ? (_MS->remaining[2] >> 4) : (_MS->remaining[2] & 0xf);
    
    // Add the available spots to the counter
    if (_1TilesLeft)
    {
        *_countPtr += popcnt12;
    }
    
    if (_2TilesLeft)
    {
        *_countPtr += popcnt12;
    }
    
    if (_3TilesLeft)
    {
#if defined (__GNUC__) || defined (__clang__)
        *_countPtr += __builtin_popcountll(avail3Mask);
#else
        *_countPtr += MakeSeven_bitCount(avail3Mask);
#endif
    }
    
    // Generate the list of available drops
    while (avail12Mask)
    {
        if (avail12Mask & MAKESEVEN_LCL)
        {
            if (_1TilesLeft)
            {
                _list[foundIndex++] = 16 | column; // (1 << 4)
            }
            
            if (_2TilesLeft)
            {
                _list[foundIndex++] = 32 | column; // (2 << 4)
            }
            
            if ((avail3Mask & MAKESEVEN_LCL) && _3TilesLeft)
            {
                _list[foundIndex++] = 48 | column; // (3 << 4)
            }
        }
        
        avail12Mask >>= MAKESEVEN_SIZE_P1;
        avail3Mask >>= MAKESEVEN_SIZE_P1;
        ++column;
    }
}

void MakeSeven_helpMessage(const char *_NAME)
{
    printf("Usage: %s <switch> [ARGUMENT]\n\nGame solver for Make 7. It is a Connect Four variant where instead of colored\n", _NAME);
    puts("discs, players drop tiles numbered 1 to 3 on a 7x7 grid. Tiles #1 and #2 can");
    puts("drop anywhere, but tile #3 can only drop in specific locations marked by red");
    puts("squares. The object is to be the first to align these tiles so that their sum");
    puts("equals 7. It is a draw if a player runs out of tiles to drop, or if the grid");
    puts("becomes full without a winner. The solver accepts the following command-line");
    puts("switches:\n");
    puts(" -h -? --help\t\tDisplays this help message and exit.\n");
    puts(" -i --interactive\tAllows the user to play interactively. They can play");
    puts("\t\t\tagainst the Monte Carlo tree search AI or an additional");
    puts("\t\t\thuman player. It is impossible to play against the");
    puts("\t\t\tminimax AI at this time.\n");
    printf(" -m --mcts\t\tUses Monte Carlo tree search instead of minimax to solve");
    puts("\n\t\t\tthe game. Cancel anytime by hitting Ctrl+C.\n");
    puts(" -p --parallel \t\tParallelizes the search at the root position. This is");
    puts("\t\t\texperimental and may not work properly in every case.\n");
    printf(" -t --table [ENTRIES]\tModifies the transposition table entry size to [ENTRIES]");
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
