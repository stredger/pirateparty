CC=gcc
CFLAGS=-Wall -c -g
LDFLAGS=-lncurses

all: p242piotest

p242piotest: p242piotest.o p242pio.o
	$(CC) p242piotest.o -o p242piotest

p242pio.o: p242pio.c p242pio.h
	$(CC) $(CFLAGS) p242pio.c 

p242piotest.o: p242piotest.c
	$(CC) $(CFLAGS) p242piotest.c 

clean:
	rm -f p242piotest.o p242piotest p242pio.o
