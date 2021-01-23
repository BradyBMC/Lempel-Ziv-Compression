# Lempel-Ziv-Compression
Data compression using the Lempel-Ziv Method
# Assignment 7

## Lempel-Ziv Compression

# by Brady Chan, brmchan 12/13/2020

## How to Run Code
To run the code type "make" in the same file directory where the code resides and hit enter to check of possibl ememory leaks. After compiling the program, type./encode or ./decode followed by a -i and -o to specify the input and output files. If no -o is supplied, in stead of writing to the file it will be printed in the terminal. -v will print out the stats of the compressed file. Type "make infer" and valgrind to check for possible memory leaks. "make clean" will them remove all temporary files created. 

## Description
Compresses and decompresses files with the Lempel-Ziv Compression method along with compression size statistics.

### Deliverables

1. Makefile
1. encode.c
1. decode.c
1. trie.c
1. trie.h
1. word.c
1. word.h
1. io.c
1. io.h
1. code.h
1. README.md
1. DESIGN.pdf

## Notes

> No make infer errors and no valgrind errors. Clears all memory successfully.
> There is edge case to detect no files put in, because the program will not run.
