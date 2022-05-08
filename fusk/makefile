CC = gcc
CFLAGS = -Wall

all: ../fisk/p242pio.o freelist.o p242pfs

#must be first thing compiled
../fisk/p242pio.o: ../fisk/p242pio.c ../fisk/p242pio.h
	$(CC) $(CFLAGS) -c ../fisk/p242pio.c
	mv p242pio.o ../fisk/p242pio.o

p242pfs: p242pfs.o freelist.o inode.o
	$(CC) $(CFLAGS) p242pfs.o inode.o freelist.o ../fisk/p242pio.o -o p242pfs

p242pfs.o: p242pfs.c
	$(CC) $(CFLAGS) -c p242pfs.c

freelist.o: freelist.c freelist.h
	$(CC) $(CFLAGS) -c freelist.c

inode.o: inode.c inode.h
	$(CC) $(CFLAGS) -c inode.c

clean:
	rm -f freelist.o p242pfs.o p242pfs testdisk inode.o
