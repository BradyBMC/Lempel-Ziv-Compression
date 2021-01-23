CFLAGS=-Wall -Wextra -Werror -Wpedantic
CC=clang $(CFLAGS)

all	:	encode decode

decode	:	decode.o io.o word.o
	$(CC) -o decode decode.o io.o word.o -lm
decode.o:	decode.c
	$(CC) -c decode.c
encode	:	encode.o trie.o io.o word.o
	$(CC) -o encode encode.o trie.o io.o word.o -lm
encode.o:	encode.c
	$(CC) -c encode.c
trie.o	:	trie.c
	$(CC) -c trie.c
io.o	:	io.c
	$(CC) -c io.c
word.o	:	word.c
	$(CC) -c word.c
clean	:
	rm -rf encode encode.o trie.o io.o word.o decode decode.o infer-out
infer	:
	make clean; infer-capture -- make; infer-analyze -- make
