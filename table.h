/*
    Copyright (C) 2020- TheTrustedComputer
    
    Transposition tables are hash tables that store intermediate results of depth-first search algorithms.
    To minimize the number of collisions, the table size is the largest prime number less than the initial size.
    It does not handle collisions; it will always overwrite the previous entry, given the same key and different value.
*/

#ifndef TABLE_H
#define TABLE_H

#include <stdlib.h>
#include <stddef.h>

#define TT_HASHSIZE 67108864
#define TT_UNKNOWN 0
#define TT_LOWERBOUND 1
#define TT_UPPERBOUND 2

// A single entry to the transposition table
typedef struct
{
    uint64_t boardKey, twoTileKey, threeTileKey;
    int value;
}
TT_TableEntry;

// The transposition table itself
typedef struct
{
    TT_TableEntry *tableEntry;
    unsigned size;
}
TranspositionTable;

// Prime number testing algorithms to minimize hash collisions
bool TranspositionTable_prime(const unsigned);                                                                      // Tests if a number is prime
unsigned TranspositionTable_prevprime(unsigned);                                                                    // Finds the largest prime number less than the input

// Memory allocation
bool TranspositionTable_initialize(TranspositionTable*, unsigned);                                                  // Initializes the transposition table
void TranspositionTable_destroy(TranspositionTable*);                                                               // Release the memory allocated to it   

// Operations on transposition tables
void TranspositionTable_storeVal(TranspositionTable*, const uint64_t, const uint64_t, const uint64_t, const int);   // Stores a key-value pair into the table
int TranspositionTable_loadVal(TranspositionTable*, const uint64_t, const uint64_t, const uint64_t);                // Loads a value from the table given a key

#endif /* TABLE_H */
