CC=gcc
CFLAGS=-Wall -std=c99 -g -O3 -I./include
CLIBS=-L./lib -lraylib -lm -lGL -lX11

cemu8: src/main.c chip8.o
	$(CC) $(CFLAGS) -o cemu8 chip8.o src/main.c $(CLIBS)

all: cemu8

chip8.o: src/chip8.c include/chip8.h
	$(CC) $(CFLAGS) -c src/chip8.c

clean:
	rm -rf *.o cemu8
