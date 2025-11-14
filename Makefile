CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g

all: chip8 parser

parser: main.c chip8.o
	$(CC) $(CFLAGS) -DPARSER -o parser chip8.o main.c

chip8: main.c chip8.o
	$(CC) $(CFLAGS) -o chip8 chip8.o main.c

chip8.o: chip8.c chip8.h
	$(CC) $(CFLAGS) -c chip8.c

clean:
	rm -rf *.o chip8 parser
