# Make-Seven
A C program to solve Make 7. It utilizes bitmaps to represent the game structure alongside necessary variables, adapted from my other project "Four the Win!", as this is the most efficient method of representation. The negamax algorithm is implemented with alpha-beta pruning, transposition table support, iterative deepening, and static move ordering to achieve fast results. Furthermore, the algorithm to check for a win uses sliding windows, a common programming technique for finding maximum sums in subarrays. There are many other optimizations that can be made to make the solver run faster. It is currently incapable of solving from the starting position in a reasonable amount of time.

An alternative called Monte Carlo tree search can be used and played against. Keep in mind that it is susceptible to shallow traps, a seemingly good short-term move becomes a bad long-term move. This is due to the nature of Monte Carlo's random sampling and cannot be fixed at its core without introducing better heuristics or domain knowledge into the algorithm. Despite this limitation, Monte Carlo tree search is highly parallizable; and unlike the minimax family, it can take advantage of today's multi-core processors.

I am open to pull requests. If you have any ideas or suggestions for improvement, please do so, and we will discuss them.


## What is Make 7?
Make 7 is a variant of Connect 4 produced by Pressman Toys. In this game, each player receives eleven 1 and 2 tiles, and four 3 tiles on a 7 by 7 playing field. The rules are that the 1's and 2's can drop to any non-full column, while the 3's can only fall into spots as indicated by the red squares. The object is to align these tiles horizontally, vertically, or diagonally so that their sum is 7. Below is a table illustrating the empty grid.

|  A  |  B  |  C  |  D  |  E  |  F  |  G  |
| --- | --- | --- | --- | --- | --- | --- | 
|  -  |  -  |  -  |  -  |  -  |  -  |  -  |
|  -  |  -  |  =  |  -  |  =  |  -  |  -  |
|  -  |  =  |  -  |  -  |  -  |  =  |  -  |
|  -  |  -  |  -  |  =  |  -  |  -  |  -  |
|  =  |  -  |  -  |  -  |  -  |  -  |  =  |
|  -  |  -  |  -  |  -  |  -  |  -  |  -  |
|  -  |  -  |  -  |  -  |  -  |  -  |  -  |

The letters A through G denote the column index, and hyphens and equal signs mark unoccupied cells available for both 1's and 2's, and  3's, respectively. Although not shown in the table, the 1's and 2's can go where the 3's go. There are 8 possible ways to add 7, given the numbers 1, 2, and 3.

1. 1+1+1+1+1+1+1
2. 2+1+1+1+1+1
3. 2+2+1+1+1
4. 2+2+2+1
5. 3+1+1+1+1
6. 3+2+1+1
7. 3+2+2
8. 3+3+1

Note that partial sums are also allowed, meaning that if a player has 3+3+1 to line up, 2+3+3+1 is considered a win. Conversely, 2+3+3+2 is not a win because there is no consecutive sequence that adds up to 7. Since addition is commutative, the sequence is interchangeable. When one player runs out of tiles to move before there is a winner, the game is declared a draw. More information about the game can be found at https://boardgamegeek.com/boardgame/6367/make-7.

## Compilation Instructions
Any C compiler will do, I recommend GCC as this is my main compiler for testing the program. To compile it, browse the directory where the sources reside and type the commands into a shell as below.

```gcc -Ofast -s -march-native main.c -o MakeSeven```

If your system has GNU Make installed, I have attached a makefile to simplify the building process for you. Type ```make```, and it will execute the same command as above. It should not take long to compile. After it completes, run the program.

```./MakeSeven```

You should see some text that looks something like this:

```
Make Seven solver by TheTrustedComputer
Hash table of 1073741789 entries
```

The number of hash table entries may vary depending on the total physical memory installed. In this diagram, the program was run on a system with 32 gigabytes of memory. It can be changed with a command-line switch ```-t <SIZE>``` , where \<SIZE> is the hash size in gigabytes.

## How to Use
To use this program, two characters encode a move. The first character specifies the type of number tile, and the second represents the column index to be dropped, using letters as described in the table. For example, the input ```2D2D1C2D``` will place three 2's in column D and a single 1 in column C. Two 2's belonging to the second player, called Yellow, will appear stacked on top of the first player's, called Green, as well as the 1 that is adjacent to it. The program alternates the colors of the letters to determine the current side to play. Below the grid is information about the remaining tiles for that side. Users may also enter moves directly from command line arguments, and they are case-insensitive.

After pressing the Enter key, the program will begin solving the current position. How it will end depends on the state of the game. If there are just a couple of tiles, it may take more than an hour to solve, or even several days, since it has a higher branching factor than Connect 4 and more complex winning logic. On the other hand, if there are many tiles, signifying the game is near its conclusion stage, solving time will be much shorter. The program will then present a solution that is formatted as follows.

```[Result] [Nodes] [Speed] [Time]```

The result can be a win (W), loss (L), or draw (D), and to the right of it is the number of moves to reach that result, in plies or half-moves, from the player's perspective. Nodes refer to the final count of game tree nodes explored. Speed measures how fast this position was solved per second. Time records the length of time in seconds spent solving. The program will then solve all possible moves for the player and randomly print one of the best moves. However, it will not solve them when given arguments. Otherwise, it will repeatedly prompt for input and solve until the user closes it.

## Bug Reports
It usually works as intended, but there may be instances where it misbehaves. Please submit any bugs you find in depth on the issues page, but understand that there is no guarantee they will be fixed in a timely manner.

## Licenses
This program is licensed under MIT. Please read it carefully before integrating mines into your own projects. The license is subject to change and will be announced accordingly.
