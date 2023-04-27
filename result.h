/*
    Copyright (C) 2020- TheTrustedComputer
    
    Stores the negamax score to a result structure.
*/

#ifndef RESULT_H
#define RESULT_H

// Shorthand macros for result values
#define WIN_TEXT "WIN"
#define LOSS_TEXT "LOSS"
#define DRAW_TEXT "DRAW"
#define NONE_TEXT "-- "
#define RESULT_DRAW (Result) { DRAW_CHAR, -1 }

// Enumeration for the result of the game: unknown, win, draw, and loss
enum ResultChar
{
    UNKNOWN_CHAR = '?', WIN_CHAR = 'W', DRAW_CHAR = 'D', LOSS_CHAR = 'L'
};

// Structure to carry the result [win/loss/draw, depth to result]
typedef struct
{
    char wdl;
    uint8_t dt7; // max depth is 49
}
Result;

void Result_print(Result*, Result*);                        // Write the result to the console
void Result_increment(Result*);                             // Increment the depth of the result
Result Result_getBestResult(Result*, Result*, Result*);     // Choose the best result from the results
uint8_t Result_getBestMove(Result*, Result*, Result*);      // Get the best move from the results

#endif /* RESULT_H */