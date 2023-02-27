# Make-Seven
A C program to solve Make 7. It utilizes bitmaps to represent the game structure alongside necessary variables, adapted from my other project "Four the Win!", as this is the most efficient method of representation. The negamax algorithm is implemented with alpha-beta pruning, transposition table support, iterative deepening, and static move ordering to achieve fast results. Furthermore, the algorithm to check for a win uses sliding windows, a common programming technique for finding maximum sums in subarrays. There are many other optimizations that can be used to have the solver run faster. If you have any ideas or suggestions for it, please open a pull request, and we will discuss them.


## What is Make 7?
Make 7 is a variant of Connect 4 currently produced by Pressman Toys. In this game, each player receives eleven 1 and 2 tiles, and four 3 tiles on a 7 by 7 playing field. The rules are that the 1's and 2's can drop to any non-full column, while the 3's can only fall into spots as indicated by the red squares. The object is to align these tiles horizontally, vertically, or diagonally so that their sum is 7. There are 8 possible ways to add 7, given the numbers 1, 2, and 3.

1. 1+1+1+1+1+1+1
2. 2+1+1+1+1+1
3. 2+2+1+1+1
4. 2+2+2+1
5. 3+1+1+1+1
6. 3+2+1+1
7. 3+2+2
8. 3+3+1

Note that partial sums are allowed, meaning that if a player has 3+3+1 lined up, 2+3+3+1 is considered a win. Conversely, 2+3+3+2 is not a win because there is no consecutive sequence that adds up to 7. Since addition is commutative, the sequence is interchangeable. When one player runs out of tiles to move before there is a winner, the game is declared a draw. More information about the game can be found at https://boardgamegeek.com/boardgame/6367/make-7.

## Compilation Instructions
Any C compiler will do, I recommend GCC as this is my main compiler for testing the program. To compile it, browse the directory where the sources reside and type the commands into a shell as shown below.

```gcc -Ofast -s -march-native main.c -o MakeSeven```

If your system has GNU Make installed, I have attached a makefile to simplify the building process for you. Enter "make" without quotes, and it will execute the same command as above.

```make```

It should not take long to compile. After it completes, run the program.

```./MakeSeven```

You should see some text that looks something like this:
```
Make Seven solver by TheTrustedComputer
Hash table of 1073741789 entries
```

The number of hash table entries may vary depending on the amount of physical memory installed. In this example, the program was run on a system with 32 gigabytes of memory. The number of hash table entries may vary depending on the amount of physical memory installed. In this diagram, the program was run on a system with 32 gigabytes of memory. It can be changed with a command-line switch ```-t <SIZE>``` , where \<SIZE> is the hash size in gigabytes.

## How to Use
To be determined.

## Bug Reports
It usually works as intended, but there may be instances where it misbehaves. Please submit any bugs you find in depth on the issues page, but understand that there is no guarantee they will be fixed in a timely manner.

## Licenses
This program is licensed under MIT. Please read it carefully before integrating mines into your own projects. The license is subject to change and will be announced accordingly.
