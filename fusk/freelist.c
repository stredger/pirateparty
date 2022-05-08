#include "freelist.h"
#include "../fisk/p242pio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BITS_PER_BLOCK (1024*8)
#define BYTE_SIZE 8
#define BLOCK_SIZE 1024
#define MAX_FREELIST_SIZE 1024

/********************************************************************************
 * freelist.c
 *
 * How it works:
 * 
 * If there are <= 1024 blocks on our disk, we will have ONE freelist.
 * If there are > 1024 blocks on our disk, we will have more than one.
 * the number of freelists = (#blocks user wants)/1024 
 * (because each block is 1024 bits, so each sub-freelist determines 
 * if 1024 blocks are free or not).
 *
 * Each sub freelist is split into 1024/ BYTE_SIZE 'letters', 
 * where each letter will represent any combination of BYTE_SIZE
 * blocks being free or not. each of these letters can be referenced by
 * freelist[letter_of_interest]. I will describe what the following code does 
        
        char to_offset
        int freelist_element = (block)/sizeof(to_offset);
        
    to_offset = to_offset<<(block)%sizeof(to_offset);
        freelist_bit_string[freelist_element] = freelist_bit_string[freelist_element] ^ to_offset;
        
 * it takes the bit 0x1, and offsets it to the left by block%BYTE_SIZE.
 * so this will be a number between 0 and 7, which references the bit of the letter
 * we want to update. then we xor that offset with the letter we want to update.
 * so for example. if we want to write to the 15th block, we go:
    
    to_offset = 00000001 shifted to the left 7 times (10000000)
    freelist_element = 15/8 = 1 (so we want the second letter [remember it is 0 based])
    
    then we xor the second letter with to_offset, flipping the bit in position 15 of the freelist
 
 * it may be wierd that we are shifting to the left, yet we are changing bit 15.
 * but we organize our freelist from RIGHT TO LEFT. so the right most bit corresponds
 * to whether or not block 0 is free. 
********************************************************************************/

void print_bits(char *bit_string) {
	int i, k, x, c = 8, m=2;
	
	printf("bitvector:\n");
	for(i=0; i<m; i++) {
		for(k=0; k<c; k++) {			
			if(bit_string[i] & (0x01 << k)) {
				x = 1;
			} else {
				x = 0;
			}
			printf("%i", x);
		}
	}
	printf("\n");
	
}

void print_freelist(freelist l) {
	freelist_node n;

	n = l->head;
	while(n != NULL) {
		printf("block num %i\n", n->block);
		l->head = l->head->next;
		n = l->head;
	}
}

void sync_freelist(int disk_fd, int block_num, int data_block_start){
        
        //OPEN FILE IN CALLING FUNCTION
        
        char freelist_bit_string[BLOCK_SIZE];
        char to_offset = 0x01;
        int freelist_element = (block_num - data_block_start) / BYTE_SIZE;

//printf("sync: blk %d\n", block_num);
//printf("sync: element %i\n", block_num/BYTE_SIZE);
//printf("sync: bit %i\n", block_num % BYTE_SIZE);

//printf("reading and writing to block %i\n", block_num/(BITS_PER_BLOCK*BYTE_SIZE) + 1);

    	//freelist_block is which freelistblock we want. it will be WRT the first free list, so +1 because 1 is the block (on fisk) of first freelist
	if(readBlock(disk_fd, block_num/(BITS_PER_BLOCK) + 1, freelist_bit_string) < 0){
		printf("Error\n");
		return;
	}
//printf("\t\tbefore char: %d\n",    freelist_bit_string[freelist_element] );
	to_offset = to_offset << (block_num % BYTE_SIZE);

//printf("\t\tto_offset: %d\n",    to_offset );

//print_bits(freelist_bit_string); 

	freelist_bit_string[freelist_element] = freelist_bit_string[freelist_element] ^ to_offset;

//printf("\t\tafter char: %d\n",    freelist_bit_string[freelist_element] );

//print_bits(freelist_bit_string); 

        if(writeBlock(disk_fd, block_num/(BITS_PER_BLOCK*BYTE_SIZE) + 1, freelist_bit_string) < 0){
               printf("Error\n");
               return;
        }

}

int get_block(int disk_fd, freelist fl, int data_block_start){
        
        freelist_node temp_node = fl->head;
        int freelist_block; //which block in the freelist we want to read the bitstring of (because we could have more than 1024 blocks, we want to
                                            //there will be (total # of blocks)/1024 freelists
        freelist_block = temp_node->block;   //   /BITS_PER_BLOCK;           
        fl->head = fl->head->next;
        free(temp_node);
        fl->count --;

        sync_freelist(disk_fd, freelist_block, data_block_start);

//printf("get: flippig block %i\n", freelist_block);
        
        if (fl->head == NULL) {
             if (populate_freelist(disk_fd, fl, fl->cur_char, data_block_start) ) {
		return -1;
		}	
        }
        
        return freelist_block;
}

int add_block(int disk_fd, freelist fl, int block_num, int data_block_start){
        
        freelist_node new_node;
        new_node = (freelist_node) malloc(sizeof(struct _freelist_node));
        
        new_node->next = NULL;
        new_node->block = block_num;
        
        fl->tail->next = new_node;
        
        sync_freelist(disk_fd, block_num, data_block_start);
        return block_num;
}

//looks through the freelist bit vector stored on fisk. Then populates the linked list with 'available' values
//this is a recursive function. When we get to the end of characters in a freelist block (on disk), we make a recursive call for the next block
//PARAMS:
//       file: the fisk to read the freelist off of
//       fl: the freelist to populate (pass by reference)
//       initial_char:  the char we first look at in the free list. used to determine if user has used up all memory
int populate_freelist(int disk_fd, freelist fl, int initial_char, int data_block_start){

	int cur_char = fl->cur_char;//keeps track of which character we are looking at (to compare vs end of freelist block)

// this seems wrong to me........
	int cur_freelist_block = cur_char/BLOCK_SIZE + 1;//which block of the freelist we are looking at

	char freelist_bit_string[BLOCK_SIZE];//the buffer to hold the bitvector
	char cur_letter;//the 8 bits (eight blocks) we are currently looking at
	
	//there are already 16 items in the freelist. return
	if(fl->count >= MAX_FREELIST_SIZE){
		return 0;
	}else{//if there are not 16 items in the freelist, we want to do some populating.
		int i = 0;//keeps track of which BIT we are looing at in the current bitstring
		
		//read the entire freelist block
		if(readBlock(disk_fd, cur_freelist_block, freelist_bit_string) < 0){
		 	fprintf(stderr,"Could not read freelist block #%d\n", cur_freelist_block);
		 	return -1;
    		}
		//loop while there are chars left in the freelist bit vector (Byte size is the same number of bits as a char)
		while(cur_char < BLOCK_SIZE){
			unsigned short shift_bit = 0x0001;//used to determine if a bit is 1 or 0

			//get the current letter (char at spot cur_char)
			cur_letter = freelist_bit_string[cur_char % BLOCK_SIZE];
			//this is where it gets tricky.
			//so loop through the bits in the char, getting ALL of them that are zero and adding that block # to the freelist.
			//do this by shifting the shift bit one to the left each time, and anding the shift bit with the string, looking for zeros
			//it will be zero only if the bit in the bitstring is zero...HARD TO EXPLAIN
			//NOTE** we can have more than 16 items in the freelist, there will be between 16 and 23, inclusive, items in it.
			//We will take all free blocks in a char, we will not stop when we have 16!!!!!! this just makes life a LOT easier
			for(i = 0; i < BYTE_SIZE; i++){
//printf("curr letter %u\n", cur_letter);	
				//printf("\t\tto_offset: %d\n",    shift_bit );
				//bitwise and the bitstring with the shift bit. if the result is zero, then that means that the bit (in bitstring) at the spot where
				//the 1 (one) is in the shift bit, is zero........(cause the shift bit contains 7 zeros and a 1)
				if(!(cur_letter & shift_bit)){//if the result is 0, there is a free block

		//	print_bits(freelist_bit_string);
					int blk_num = (cur_freelist_block - 1)*BITS_PER_BLOCK + BYTE_SIZE*cur_char + i + data_block_start;//get the block number
					
					freelist_node new_node = malloc(sizeof(struct _freelist_node));//create a new node
					new_node->next = NULL;
					new_node->block = blk_num;
					
					//if the head is null, there is nothing in the list. set the head
					if(fl->head == NULL){
						fl->head = new_node;
						fl->tail = new_node;
					}else{//else just tack the new node onto the tail
						fl->tail->next = new_node;
						fl->tail = new_node;
					}
			
					fl->count++;
					
				}
//printf("shift %u\n", shift_bit);
				shift_bit = shift_bit << 1;//shift to the left by 1
			}
			cur_char++;//once we go through all bits in the char, increment the current char
			//ok. so if we get back to the initial char AND there are no items in the freelist, the user used up all their disk space
			if(((cur_char + BITS_PER_BLOCK*(cur_freelist_block-1)) == initial_char) && !fl->count){
				fprintf(stderr, "You landlubber, you ran out of disk space!\n");
				return -1;
			}		

			if(fl->count >= MAX_FREELIST_SIZE) break;//once we hit 16 items, just break out.
			
		}
		
		fl->cur_char = cur_char;//set the cur char
		if(populate_freelist(disk_fd, fl, initial_char, data_block_start) ) { //make the recursive call
			return -1;
		}
	}
	return 0;
}


freelist create_new_freelist(int disk_fd, int freelist_blocks, int cur_char, int data_block_start){
	
	freelist fl = malloc(sizeof(struct _freelist));
	
	fl->head = NULL;
	fl->tail = NULL;
	fl->count = 0;
	fl->cur_char = cur_char;
	fl->num_freelists = freelist_blocks;
	
	populate_freelist(disk_fd, fl, cur_char, data_block_start);
	
	return fl;
}
	
	
void destroy_freelist(freelist l) {
	freelist_node n;

	n = l->head;
	while(n != NULL) {	
		l->head = l->head->next;
		free(n);
		n = l->head;
	}
	free(l);
}
	
	
	
	/*
	int words_per_freelist = BITS_PER_BLOCK/BYTE_SIZE
	int num_freelists = fl->num_freelists;
	int freelist_pos = fl->cur_word;
	int cur_freelist = (num_freelists + 1) - ceiling(cur_word/words_per_freelist);
	char freelist_bit_string[BITS_PER_BLOCK];	
	char cur_word;
	
	if(readBlock(file, cur_freelist, freelist_bit_string) < 0){
     	printf("Error\n");
     	return;
    }
	
	freelist_pos = freelist_pos % (words_per_freelist);//update to get the freelist pos in the CURRENT freelist

	while(freelist_pos < words_per_freelist){

		cur_word = freelist_bit_string[(words_per_freelist - 1) - freelist_pos];
		int j = 0;
		char shift_bit = 0x01;
		for(j = 0; j < 8; j++){

			if(!(cur_word & shift_bit)){
				freelist_node node = malloc(sizeof(struct _freelist_node));
			}
		
			shift_bit = shift_bit << 1;
			
		}	
		
		freelist_pos++;
	
	}*/
	
	
	
	












