#ifndef TRANSPOSITIONTABLE_H
#define TRANSPOSITIONTABLE_H

#define TT_HASHSIZE 67108864
#define TT_LOWERBOUND 1
#define TT_UPPERBOUND 2

typedef struct {
	uint64_t boardKey, twoTileKey, threeTileKey;
	int value, bounds;
} TT_TableEntry;

typedef struct {
	TT_TableEntry *tableEntry;
	unsigned size;
} TranspositionTable;

// Prime number testing algorithms to minimize hash collisions
bool prime(unsigned);
unsigned prevprime(unsigned);

bool TranspositionTable_initialize(TranspositionTable*, unsigned);
void TranspositionTable_destroy(TranspositionTable*);
void TranspositionTable_reset(TranspositionTable*);

void TranspositionTable_store(TranspositionTable*, uint64_t, uint64_t, uint64_t, int);
void TranspositionTable_storeBounds(TranspositionTable*, uint64_t, uint64_t, uint64_t, int, int);
int TranspositionTable_load(TranspositionTable*, uint64_t, uint64_t, uint64_t);
int TranspositionTable_loadBounds(TranspositionTable*, uint64_t, uint64_t, uint64_t);


#endif
