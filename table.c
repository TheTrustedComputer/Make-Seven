/*
    Copyright (C) 2020- TheTrustedComputer
*/

#include "table.h"

bool TranspositionTable_prime(const unsigned _N)
{
    unsigned i;
    
    // Zero and one are not considered prime numbers
    if (!_N || (_N == 1))
    {
        return false;
    }
    
    // Two and three are prime
    if (_N <= 3)
    {
        return true;
    }
    
    // Test divisibility by 2 and 3
    if (!((_N % 2) && (_N % 3)))
    {
        return false;
    }
    
    // Test divisibility by 6i +/- 1 for i < sqrt(n)
    for (i = 5 ; i * i <= _N; i += 6)
    {
        if (!(_N % i) || !(_N % (i + 2)))
        {
            return false;
        }
    }
    
    return true;
}

unsigned TranspositionTable_prevprime(unsigned _n)
{
    _n = (_n & 1) ? _n - 2 : _n - 1;
    
    while (!TranspositionTable_prime(_n))
    {
        _n -= 2;
    }
    
    return _n;
}

bool TranspositionTable_initialize(TranspositionTable *_tt, unsigned _initSize)
{
    if (_initSize > 3)
    {
        _tt->size = TranspositionTable_prevprime(_initSize);
        
        if (!(_tt->tableEntry = malloc(sizeof(*_tt->tableEntry) * _tt->size)))
        {
            _tt->size = 0;
            return false;
        }
        
        return true;
    }
    
    return false;
}

void TranspositionTable_destroy(TranspositionTable* _tt)
{
    free(_tt->tableEntry);
    _tt->tableEntry = NULL;
}

void TranspositionTable_storeVal(TranspositionTable *_tt, const uint64_t _B_KEY, const uint64_t _KEY2, const uint64_t _KEY3, const int _VAL)
{
    unsigned i = _B_KEY % _tt->size;
    
    _tt->tableEntry[i].boardKey = _B_KEY;
    _tt->tableEntry[i].twoTileKey = _KEY2;
    _tt->tableEntry[i].threeTileKey = _KEY3;
    _tt->tableEntry[i].value = _VAL;
}

int TranspositionTable_loadVal(TranspositionTable *_tt, const uint64_t _B_KEY, const uint64_t _KEY2, const uint64_t _KEY3)
{
    unsigned i = _B_KEY % _tt->size;
    
    if ((_tt->tableEntry[i].boardKey == _B_KEY) && (_tt->tableEntry[i].twoTileKey == _KEY2) && (_tt->tableEntry[i].threeTileKey == _KEY3))
    {
        return _tt->tableEntry[i].value;
    }
    
    return TT_UNKNOWN;
}
