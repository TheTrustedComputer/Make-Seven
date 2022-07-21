#include "transpositiontable.h"

bool prime(unsigned n) {
	unsigned i;
	if (!(n & 1)) {
		return false;
	}
	for (i = 3; i * i <= n; i += 2) {
		if (!(n % i)) {
			return false;
		}
	}
	return true;
}

unsigned prevprime(unsigned n) {
	n = n & 1 ? n - 2 : n - 1;
	while (!prime(n)) {
		n -= 2;
	}
	return n;
}

void TranspositionTable_reset(TranspositionTable* tt) {
	memset(tt->tableEntry, 0, sizeof(TT_TableEntry) * tt->size);
}

bool TranspositionTable_initialize(TranspositionTable *tt, unsigned initSize) {
	if (initSize > 3) {
		tt->size = prevprime(initSize);
		if (!(tt->tableEntry = malloc(sizeof(TT_TableEntry) * tt->size))) {
			tt->size = 0;
			return false;
		}
		return true;
	}
	return false;
}

void TranspositionTable_destroy(TranspositionTable* tt) {
	free(tt->tableEntry);
	tt->tableEntry = NULL;
}

void TranspositionTable_store(TranspositionTable *tt, uint64_t key, uint64_t key2, uint64_t key3, int value) {
	unsigned i = key % tt->size;
	tt->tableEntry[i].boardKey = key;
	tt->tableEntry[i].twoTileKey = key2;
	tt->tableEntry[i].threeTileKey = key3;
	tt->tableEntry[i].value = value;
}

void TranspositionTable_storeBounds(TranspositionTable *tt, uint64_t key, uint64_t key2, uint64_t key3, int value, int bounds) {
	unsigned i = key % tt->size;
	tt->tableEntry[i].boardKey = key;
	tt->tableEntry[i].twoTileKey = key2;
	tt->tableEntry[i].threeTileKey = key3;
	tt->tableEntry[i].value = value;
	tt->tableEntry[i].bounds = bounds;
}

int TranspositionTable_load(TranspositionTable *tt, uint64_t key, uint64_t key2, uint64_t key3) {
	unsigned i = key % tt->size;
	if (tt->tableEntry[i].boardKey == key && tt->tableEntry[i].twoTileKey == key2 && tt->tableEntry[i].threeTileKey == key3) {
		return tt->tableEntry[i].value;
	}
	return 0;
}

int TranspositionTable_loadBounds(TranspositionTable *tt, uint64_t key, uint64_t key2, uint64_t key3) {
	unsigned i = key % tt->size;
	if (tt->tableEntry[i].boardKey == key && tt->tableEntry[i].twoTileKey == key2 && tt->tableEntry[i].threeTileKey == key3) {
		return tt->tableEntry[i].bounds;
	}
	return 0;
}
