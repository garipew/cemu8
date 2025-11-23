CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g -I/usr/local/include/snorkel -L/usr/local/lib
CLIBS=-lraylib -lsnorkel -lpthread

chip8: main.c chip8.o
	$(CC) $(CFLAGS) -o chip8 chip8.o main.c $(CLIBS)

all: chip8 parser sanitize

sanitize: main.c chip8.o
	$(CC) $(CFLAGS) -o sanitize chip8.o main.c $(CLIBS)

parser: main.c chip8.o
	$(CC) $(CFLAGS) -DPARSER -o parser chip8.o main.c $(CLIBS)

chip8.o: chip8.c chip8.h
	$(CC) $(CFLAGS) -c chip8.c

clean:
	rm -rf *.o chip8 parser sanitize
