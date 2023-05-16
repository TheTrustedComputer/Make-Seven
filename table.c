/*
    Copyright (C) 2020- TheTrustedComputer
*/

#include "table.h"

bool TransTable_prime(const size_t _N)
{
    size_t i;
    
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

size_t TransTable_prevprime(size_t _n)
{
    _n = (_n & 1) ? _n - 2 : _n - 1;
    
    // Subtract by 2 until it is prime
    while (!TransTable_prime(_n))
    {
        _n -= 2;
    }
    
    return _n;
}

bool TransTable_initialize(TransTable *_tt, const size_t _INIT_SIZE)
{
    size_t entryAlloc;
    bool success = false;
    
    if (_INIT_SIZE > 3)
    {
        _tt->size = TransTable_prevprime(_INIT_SIZE);
        entryAlloc = sizeof(*_tt->entry) * _tt->size;
        
        if ((_tt->entry = malloc(entryAlloc)))
        {
            //memset(_tt->entry, 0, entryAlloc);
            success = true;
        }
    }
    
    return success;
}

void TransTable_destroy(TransTable *_tt)
{
    free(_tt->entry);
    _tt->entry = NULL;
}

void TransTable_store(TransTable *_tt, const uint64_t _G_KEY, const uint64_t _KEY2, const uint64_t _KEY3, const int _VAL)
{
    size_t i = _G_KEY % _tt->size;
    
    _tt->entry[i].gridKey = _G_KEY;
    _tt->entry[i].twoKey = _KEY2;
    _tt->entry[i].threeKey = _KEY3;
    _tt->entry[i].value = _VAL;
}

int TransTable_load(TransTable *_tt, const uint64_t _G_KEY, const uint64_t _KEY2, const uint64_t _KEY3)
{
    size_t i = _G_KEY % _tt->size;
    
    return (_tt->entry[i].gridKey == _G_KEY) && (_tt->entry[i].twoKey == _KEY2) && (_tt->entry[i].threeKey == _KEY3) ? _tt->entry[i].value : TT_UNKNOWN;
}
