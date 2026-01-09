CC=zig cc
CFLAGS=-Wall -Wextra -std=c99 -g -O3
CLIBS=-Iinclude -I/usr/local/include/snorkel -L/usr/local/lib -lraylib -lsnorkel

cemu8: src/main.c chip8.o
	$(CC) $(CFLAGS) -o cemu8 chip8.o src/main.c $(CLIBS)

all: cemu8

chip8.o: src/chip8.c include/chip8.h
	$(CC) $(CFLAGS) -c -Iinclude src/chip8.c

clean:
	rm -rf *.o cemu8
