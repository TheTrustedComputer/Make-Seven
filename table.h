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
    uint64_t gridKey, twoKey, threeKey;
    int value;
}
TT_Entry;

// The transposition table itself
typedef struct
{
    TT_Entry *entry;
    size_t size;
}
TransTable;

// Prime number testing algorithms to minimize hash collisions
bool TransTable_prime(const size_t);                                                                    // Tests if a number is prime
size_t TransTable_prevprime(size_t);                                                                    // Finds the largest prime number less than the input

// Memory allocation
bool TransTable_initialize(TransTable*, const size_t);                                                  // Initializes the transposition table
void TransTable_destroy(TransTable*);                                                                   // Release the memory allocated to it   

// Operations on transposition tables
void TransTable_store(TransTable*, const uint64_t, const uint64_t, const uint64_t, const int);          // Stores a key-value pair into the table
int TransTable_load(TransTable*, const uint64_t, const uint64_t, const uint64_t);                       // Loads a value from the table given a key

#endif /* TABLE_H */
