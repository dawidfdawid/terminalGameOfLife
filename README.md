# terminalGameOfLife

This is Conway's Game of Life in your terminal.
The single threaded version is for viewing the evolution of cells generation by generation for x generations.
The multi-threaded version is for calculating the final board after x generations. 
How to use: after you make the file just type this into the console, for the multi threaded version: ./liveMT board.txt x y
for the single threaded, its just:./live board.txt x
where board.txt is the text file containing the board, I include an example board. X is the number of generations and y is the number of threads being used.
The multi threaded version only works in linux because I use the default implementation of pthread barriers, which macOS doesn't include.
