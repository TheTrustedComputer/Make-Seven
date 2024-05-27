/*
    Copyright (C) 2020- TheTrustedComputer
*/

#include "result.h"

void Result_print(const Result *_currR, const Result *_bestR)
{
#ifdef _WIN32 
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
    switch (_currR->wdl)
    {
    default:
        printf(NONE_TEXT);
        break;
    case DRAW_CHAR:
        if (_bestR && _bestR->wdl == _currR->wdl)
        {
#ifdef _WIN32 
            SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            printf("%c ", DRAW_CHAR);
            SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
            printf("\e[1;33m%c\e[0m ", DRAW_CHAR);
#endif
        }
        else
        {
#ifdef _WIN32 
            SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN);
            printf("%c ", DRAW_CHAR);
            SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
            printf("\e[0;33m%c\e[0m ", DRAW_CHAR);
#endif
        }
        break;
    case WIN_CHAR:
        if (_currR->dt7)
        {
            if (_bestR && _bestR->dt7 == _currR->dt7 && _bestR->wdl == _currR->wdl)
            {
#ifdef _WIN32 
                SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                printf("%c%d ", _currR->wdl, _currR->dt7);
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                printf("\e[1;32m%c%d\e[0m ", _currR->wdl, _currR->dt7);
#endif
            }
            else
            {
#ifdef _WIN32 
                SetConsoleTextAttribute(handle, FOREGROUND_GREEN);
                printf("%c%d ", _currR->wdl, _currR->dt7);
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                printf("\e[0;32m%c%d\e[0m ", _currR->wdl, _currR->dt7);
#endif
            }
        }
        else
        {
            if (_bestR && _bestR->dt7 == _currR->dt7 && _bestR->wdl == _currR->wdl)
            {
#ifdef _WIN32
                SetConsoleTextAttribute(handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                printf("%s ", WIN_TEXT);
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                printf("\e[1;32m%s\e[0m ", WIN_TEXT);
#endif
            }
            else
            {
#ifdef _WIN32
                SetConsoleTextAttribute(handle, FOREGROUND_GREEN);
                printf("%s ", WIN_TEXT);
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                printf("\e[0;32m%s\e[0m ", WIN_TEXT);
#endif
            }
        }
        break;
    case LOSS_CHAR:
        if (_currR->dt7)
        {
            if (_bestR && _bestR->dt7 == _currR->dt7 && _bestR->wdl == _currR->wdl)
            {
#ifdef _WIN32
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
                printf("%c%d ", _currR->wdl, _currR->dt7);
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                printf("\e[1;31m%c%d\e[0m ", _currR->wdl, _currR->dt7);
#endif
            }
            else
            {
#ifdef _WIN32
                SetConsoleTextAttribute(handle, FOREGROUND_RED);
                printf("%c%d ", _currR->wdl, _currR->dt7);
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                printf("\e[0;31m%c%d\e[0m ", _currR->wdl, _currR->dt7);
#endif
            }
        }
        else
        {
            if (_bestR && _bestR->dt7 == _currR->dt7 && _bestR->wdl == _currR->wdl)
            {
#ifdef _WIN32
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
                printf("%s ", LOSS_TEXT);
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                printf("\e[1;31m%s\e[0m ", LOSS_TEXT);
#endif
            }
            else
            {
#ifdef _WIN32
                SetConsoleTextAttribute(handle, FOREGROUND_RED);
                printf("%s ", LOSS_TEXT);
                SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
                printf("\e[0;31m%s\e[0m ", LOSS_TEXT);
#endif
            }
        }
    }
}

void Result_increment(Result *_r)
{
    switch (_r->wdl)
    {
    case WIN_CHAR:
        _r->wdl = LOSS_CHAR;
        _r->dt7++;
        break;
    case LOSS_CHAR:
        _r->wdl = WIN_CHAR;
        _r->dt7++;
    }
}

Result Result_getBestResult(const Result *_r1, const Result *_r2, const Result *_r3)
{
    unsigned i, score;
    Result best = { LOSS_CHAR, 0 };
    
    assert(_r1 && _r2 && _r3);
    
    for (i = score = 0; i < MAKE7_SIZE;)
    {
        switch (score)
        {
        case 0:
            switch (_r1[i].wdl)
            {
            case LOSS_CHAR:
                if (_r1[i].dt7 > best.dt7)
                {
                    best.dt7 = _r1[i].dt7;
                }
                break;
            case DRAW_CHAR:
            case WIN_CHAR:
                best = _r1[i];
                score = _r1[i].wdl == DRAW_CHAR ? 1 : 2;
                continue;
            }            
            switch (_r2[i].wdl)
            {
            case LOSS_CHAR:
                if (_r2[i].dt7 > best.dt7)
                {
                    best.dt7 = _r2[i].dt7;
                }
                break;
            case DRAW_CHAR:
            case WIN_CHAR:
                best = _r2[i];
                score = _r2[i].wdl == DRAW_CHAR ? 1 : 2;
                continue;
            }         
            switch (_r3[i].wdl)
            {
            case LOSS_CHAR:
                if (_r3[i].dt7 > best.dt7)
                {
                    best.dt7 = _r3[i].dt7;
                }
                break;
            case DRAW_CHAR:
            case WIN_CHAR:
                best = _r3[i];
                score = _r3[i].wdl == DRAW_CHAR ? 1 : 2;
                continue;
            }
            break;
        case 1:
            switch (_r1[i].wdl)
            {
            case WIN_CHAR:
                best = _r1[i];
                score = 2;
            }            
            switch (_r2[i].wdl)
            {
            case WIN_CHAR:
                best = _r2[i];
                score = 2;
            }          
            switch (_r3[i].wdl)
            {
            case WIN_CHAR:
                best = _r3[i];
                score = 2;
            }
            break;
        case 2:
            if ((_r1[i].wdl == WIN_CHAR) && (_r1[i].dt7 < best.dt7))
            {
                best.dt7 = _r1[i].dt7;
            }
            if ((_r2[i].wdl == WIN_CHAR) && (_r2[i].dt7 < best.dt7))
            {
                best.dt7 = _r2[i].dt7;
            }
            if ((_r3[i].wdl == WIN_CHAR) && (_r3[i].dt7 < best.dt7))
            {
                best.dt7 = _r3[i].dt7;
            }
        }
        
        i++;
    }
    
    return best;
}

uint8_t Result_getBestMove(const Result *_r1, const Result *_r2, const Result *_r3)
{
    unsigned randBest;
    int i, j, bestIdx;
    uint8_t bestTile[MAKE7_SIZE_X3], bestCol[MAKE7_SIZE_X3];
    Result bestResult = Result_getBestResult(_r1, _r2, _r3);
    bestIdx = 0;
    
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < MAKE7_SIZE; j++)
        {
            switch (i)
            {
            case 0:
                if ((_r1[j].wdl != UNKNOWN_CHAR) && (bestResult.dt7 == _r1[j].dt7))
                {
                    bestTile[bestIdx] = 1;
                    bestCol[bestIdx++] = j;
                }
                break;
            case 1:
                if ((_r2[j].wdl != UNKNOWN_CHAR) && (bestResult.dt7 == _r2[j].dt7))
                {
                    bestTile[bestIdx] = 2;
                    bestCol[bestIdx++] = j;
                }
                break;
            case 2:
                if ((_r3[j].wdl != UNKNOWN_CHAR) && (bestResult.dt7 == _r3[j].dt7))
                {
                    bestTile[bestIdx] = 3;
                    bestCol[bestIdx++] = j;
                }
            }
        }
    }
    
    assert(bestIdx);
    randBest = genrand_int32() % bestIdx;
    
    return (bestTile[randBest] << 4) | (bestCol[randBest]);
}
