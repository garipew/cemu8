CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g -I/usr/local/include/snorkel -L/usr/local/lib

all: chip8 parser

chip8: main.c chip8.o
	$(CC) $(CFLAGS) -o chip8 chip8.o main.c -lraylib -lsnorkel

parser: main.c chip8.o
	$(CC) $(CFLAGS) -DPARSER -o parser chip8.o main.c -lraylib -lsnorkel

chip8.o: chip8.c chip8.h
	$(CC) $(CFLAGS) -c chip8.c

clean:
	rm -rf *.o chip8 parser
