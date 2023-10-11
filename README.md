make to compile the program

major2 executable name

make clean to remove executable and .o files

Design Overview:
Running the program with no arguments will put it in interactive mode where the user can change directory, look at their history, and other things.
Multiple commands can be put on the same line if they are separated by a semicolon.
The program can be run in batch mode if a file is given as an argument on the command line.
Batch mode is treated the same as interactive mode but input is taken from a file instead.

Support builtin cd and history

