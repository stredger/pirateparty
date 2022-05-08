/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
*/

#include "p242pio.h"

#define MAX_INPUT_CHARS 256
#define BLOCK_SIZE 1024

int curDisk;
int curBlocks;
void closeDisk();

// function to close open disk
void closeDisk() {
	if(close(curDisk)) {
		perror("Failed to close fd");
	}
}

// FUCK THIS SHIT I CANT FUCKING PUSH MY CHANGES


int openDisk(char *filename, int nbytes) {

	if(nbytes < 0) {
		fprintf(stderr, "Invalid file size: File cannot have a negative number of bytes\n");
		return -1;
	}

	// dont allow . or .. as disk names
	if(!strcmp(filename, ".\0") || !strcmp(filename, "..\0")){
		fprintf(stderr, "Invalid filename\n");
		return -1;
	}

	// Set the # of blocks for the current file as the ceiling of: # bytes / block size.
	curBlocks = (nbytes % BLOCK_SIZE) ? nbytes/BLOCK_SIZE + 1 : nbytes/BLOCK_SIZE;
	
	// open a file for reading/writing, create it if non-existent
	if( (curDisk = open(filename, O_RDWR|O_CREAT, 0666)) == -1 ) {
		perror("Failed to open file");
		return -1;
	}
	// register closeDisk to run upon process termination
	if(atexit((void *) &closeDisk) ) {
		fprintf(stderr, "Failed to register exit function\n");
	}
	return curDisk;
}


int readBlock(int disk, int blocknr, void *block) {
	int bytes_read;
	
	if(disk >= 0){
		if( blocknr >= curBlocks ) {
			fprintf(stderr, "Block %i doesn't exist\n", blocknr);
		} else {
			if(lseek(disk, blocknr*BLOCK_SIZE, SEEK_SET) == -1) {
				perror("lseek failed to set file position");
				return -1; //lseek failed
			}
			if( (bytes_read = read(disk,(char*)block,BLOCK_SIZE)) == -1) {
				perror("Failed to read block from disk");
			}
			return bytes_read; // return # bytes read or -1 on error
		}
	}
	return -1; //disk file descriptor < 0
}


int writeBlock(int disk, int blocknr, void *block) {
	int num_bytes;
	
	if(disk >= 0){ // dont want true on -1
		if( blocknr >= curBlocks ) {
			fprintf(stderr, "Block %i doesn't exist\n", blocknr);
		} else {
			if(lseek(disk, blocknr*BLOCK_SIZE, SEEK_SET) == -1) {
				perror("lseek failed to set file position");
				return -1; //lseek failed			
			}

			if((num_bytes = write(disk,block,BLOCK_SIZE)) == -1) {
				perror("Failed to write block to disk");
			}
			return num_bytes;
		}
	}
	return -1;
}


void syncDisk() {
	fsync(curDisk);
}










