
#include "inode.h"


// this should be in a global include file!!!!!!/////////////////

// Calculate the ceiling of X / Y
int _ceiling (int x, int y) {
	return (x % y) ? x / y + 1 : x / y;
}
//////////////////////////////////////////////////////////////////



void print_inode_list(inode_list ilist) {
	inode_list_node n;
	n = ilist->head;
	while(n != NULL) {
		printf("inode: %i\n", n->num);
		n = n->next;
	}
}


void print_dir(dir *dirs, int num_blks) {
	int i, j;

	printf("inode\t:\tname\n");

	for(i = 0; i<num_blks; i++) {
		for(j = 0; j<MAX_DIR_ENTRIES; j++) {
			printf("%i\t:\t%s\n", dirs[i]->inode_num[j], dirs[i]->name[j]);
		}
	}
}


void print_inode(inode n) {
	int i;
	printf("num:\t%i", n->num);
	printf("type:\t%i", (int) n->type);
	printf("size:\t%i", n->size);
	printf("Blocklist:\n");
	for(i = 0; i<INODE_BLKLIST_SIZE; i++) {
		printf("%i: ", n->block_list[i]);
	}
	printf("\n");
}


int create_inode(int num, inode_list ilist) {
	inode_list_node node;

	if( !(node = (inode_list_node) malloc(sizeof(struct _inode_list_node))) ) {
		fprintf(stderr, "Failed to allocate memory for inode\n");
		return -1;
	}
	node->next = NULL;
	node->num = num;

	if(ilist->head == NULL) {
		ilist->tail = ilist->head = node;
	} else {
		ilist->tail->next = node;
		ilist->tail = node;
	}

	return 0;
}


int get_inode(inode_list ilist) {
	int num = 0;
	inode_list_node node;

	if(ilist->head == NULL) {
		fprintf(stderr, "the inode_list be empty matey\n");
		return -1;
	}

	node = ilist->head;
	ilist->head = ilist->head->next;
	num = node->num;
	free(node);

	return num;
}


int fill_inode_list(int disk_fd, inode_list ilist, int first_inode_block, int inode_blocks) {
	int i, j, inodes_per_block, bytes_read;
	char buffer[BLOCK_SIZE];
	struct _inode *inode_arr;

	inodes_per_block = BLOCK_SIZE / sizeof(struct _inode);

	for(i = 0; i<inode_blocks; i++) {
		if( !(bytes_read = readBlock(disk_fd, i + first_inode_block, (void *) buffer)) ) {
			for(j = 0; j<inodes_per_block; j++) {
				if(create_inode( (i*inodes_per_block) + j, ilist) == -1) {
					return -1;
				}
			}	
		} else if(bytes_read == -1) {
			return -1;
		} else {
			inode_arr = (inode) buffer; 
			for(j = 0; j<inodes_per_block; j++) {
				if(inode_arr[j].type == 0) {
					if(create_inode( (i*inodes_per_block) + j, ilist) == -1) {
						return -1;
					}
				}
			}
		}
	}

	return 0;
}



int update_inode_block(int disk_fd, inode node, int inode_block_start) {
	char buffer[BLOCK_SIZE];
	int inodes_per_block, bytes_read, inode_block, inode_index;
	struct _inode *inode_arr;

	inodes_per_block = BLOCK_SIZE / sizeof(struct _inode);
	inode_block = node->num / inodes_per_block + inode_block_start;
	inode_index = node->num % inodes_per_block;

//printf("Updating inode block %i\n", inode_block);
	
	if( !(bytes_read = readBlock(disk_fd, inode_block, buffer)) ) {
		memset(buffer, 0x00, BLOCK_SIZE*sizeof(char));
	} else if(bytes_read == -1) {
		fprintf(stderr, "Failed to read block %i", inode_block);
		return -1;
	}

	inode_arr = (inode) buffer;
	memcpy(&inode_arr[inode_index], node, sizeof(struct _inode));

	if( writeBlock(disk_fd, inode_block, buffer) == -1) {
		fprintf(stderr, "Failed to read block %i", inode_block);
		return -1;
	}
	syncDisk();

	return 0;
}



inode_list create_inode_list(int disk_fd, int first_inode_block, int inode_blocks) {
	inode_list ilist;

	if( !(ilist = (inode_list) malloc(sizeof(struct _inode_list))) ) {
		fprintf(stderr, "failed to allocate memory for inode list\n");
		return NULL;
	}
	memset(ilist, 0x00, sizeof(struct _inode_list));

	if(fill_inode_list(disk_fd, ilist, first_inode_block, inode_blocks) == -1) {
		fprintf(stderr, "failed to fill inode list\n");
		return NULL;
	}
	return ilist;
}



void destroy_inode_list(inode_list ilist) {
	inode_list_node n;

	n = ilist->head;
	while(n != NULL) {
		ilist->head = ilist->head->next;
		free(n);
		n = ilist->head;
	}
	free(ilist);
}



int read_partial_block(int disk_fd, int block, int num_bytes, void *data) {
	char buff[BLOCK_SIZE];

	memset(buff, 0x00, BLOCK_SIZE*sizeof(char));
	if(readBlock(disk_fd, block, (void *) buff) == -1) {
		fprintf(stderr, "Failed to write partial block\n");
		return -1;
	}
	memcpy(data, buff, num_bytes);
	return 0;
}



int read_indir_data(int *blocks_remaining, int f_size, int num_blocks, int block, 
			char *data, int disk_fd, int indir_lvl, int data_start) {
	int i, indir_list[BLOCK_SIZE/sizeof(int)];

	if( (*blocks_remaining) < 1 ) {
		return 0;
	}

printf("READ: indir level: %i\n", indir_lvl);

	if(indir_lvl == 0) {
		if( readBlock(disk_fd, block, (void *) indir_list) == -1 ) {
			return -1;
		}
		for(i=0; i<BLOCK_SIZE/sizeof(int); i++) {

			if( (*blocks_remaining) < 1 ) {
				return 0;
			}

			if( (block = indir_list[i]) < data_start ) { 
				fprintf(stderr, "trying to access non data block, bail!\n");
				return -1;
			}

//printf("READ: indir block: %i\n", block);

			if( *blocks_remaining == 1 && f_size % BLOCK_SIZE ) {
				if( read_partial_block(disk_fd, block, f_size % BLOCK_SIZE, (void *) &data[(num_blocks - *blocks_remaining)*BLOCK_SIZE]) == -1) {
					fprintf(stderr, "Failed to read block\n");
					return -1;
				}
			} else if( readBlock(disk_fd, block, (void *) &data[(num_blocks - *blocks_remaining)*BLOCK_SIZE]) == -1) {
				fprintf(stderr, "Failed to read file\n");
				return -1;
			}
			if( ((*blocks_remaining)--) < 1 ) {
				return 0;
			}
		}
	} else if(indir_lvl > 0){ 

		if( readBlock(disk_fd, block, (void *) indir_list) == -1 ) {
			return -1;
		}

		for(i = 0; i < BLOCK_SIZE/sizeof(int); i++){

			// get block for next indirection level
			if( (block = indir_list[i]) < data_start ) { 
				fprintf(stderr, "trying to access non data block, bail!\n");
				return -1;
			}
			// perform next indirection level
			if( read_indir_data(blocks_remaining, f_size, num_blocks, block ,data, disk_fd, indir_lvl - 1, data_start) == -1) {
				return -1;
			}
			if( (*blocks_remaining) < 1 ) { // dont decrement (no data read)
				break;
			}
		}
	  	return 0;
	} else {
		fprintf(stderr, "invalid indir level %i\n", indir_lvl);
	}

	return 0;
}



char *read_file(fs_metadata metadata, inode node) {
	char *data;
	int i, num_blocks, blocks_remaining, ind_level, iters, block, disk_fd;

	disk_fd = metadata->disk_fd;

	if( !(data = malloc(node->size*sizeof(char))) ) {
		fprintf(stderr, "failed to allocate memory for file\n");
		return NULL;
	}

	blocks_remaining = num_blocks = _ceiling(node->size, BLOCK_SIZE); // We need to change this when we put into global

	// read first ten blocks ()
	iters = (num_blocks < 11) ? num_blocks: 10;
	for(i = 0; i<iters; i++) {
		if( (block = node->block_list[i]) < metadata->data_start ) {
			fprintf(stderr, "trying to access non data block\n");
			free(data);
			return NULL;
		}

//printf("READ: non-indir, reading blk: %i\n", block);

		if(blocks_remaining == 1 && node->size % BLOCK_SIZE ) {
			if( read_partial_block(disk_fd, block, node->size % BLOCK_SIZE, (void *) &data[i*BLOCK_SIZE]) == -1) {
				fprintf(stderr, "Failed to read block\n");
				free(data);
				return NULL;
			}
		} else if( readBlock(disk_fd, block, (void *) &data[i*BLOCK_SIZE]) == -1) {
			fprintf(stderr, "Failed to read block\n");
			free(data);
			return NULL;
		}
		blocks_remaining--;	
	}

	ind_level = 0;
 	while(blocks_remaining > 0) {
 
 		if(ind_level >= 3) {
 			fprintf(stderr, "Ye file be too big\n");
			free(data);
			return NULL;
 		}
 
		if( (block = node->block_list[i + ind_level]) < metadata->data_start ) {
			fprintf(stderr, "trying to access non data block\n");
			free(data);
			return NULL;
		}

//printf("READ: indir blk: %i\n", block);

 		if( read_indir_data(&blocks_remaining, node->size, num_blocks, block, data, disk_fd, 
					ind_level,  metadata->data_start) == -1) {
			free(data);
			return NULL;
		}

 		ind_level++;
 	}

	return data;
}


int write_partial_block(int disk_fd, int block, int num_bytes, void *data) {
	char buff[BLOCK_SIZE];

	memset(buff, 0x00, BLOCK_SIZE*sizeof(char));
	memcpy(buff, data, num_bytes);
	if(writeBlock(disk_fd, block, (void *) buff) == -1) {
		fprintf(stderr, "Failed to write partial block\n");
		return -1;
	}	
	return 0;
}

 
int *add_indir_data(int *blocks_remaining, int f_size, int num_blocks, char *data, 
			int disk_fd, int indir_lvl, freelist data_freelist, int data_start){
	int i;
	int *ind_blocks, *ind_list;

	if( !(*blocks_remaining) ) {
		return NULL;
	}

printf("WRITE: indir level %i\n", indir_lvl);

	if(indir_lvl == 0) {

		// we are at the last of the indirection blocks, so allocate memory to hold the "leaf" block numbers
		if( !(ind_blocks = (int *) calloc(BLOCK_SIZE/sizeof(int), sizeof(int))) ) {
			fprintf(stderr, "unable to allocate memory for indirection lv %i\n", indir_lvl);
			return NULL;
		}

		for(i = 0; i < BLOCK_SIZE/sizeof(int); i++) {
			if( (*blocks_remaining) < 1 ) {
				break;
			}

//printf(":::WRITE: blocks remaining %i\n", *blocks_remaining);

			// get block where data will be written to
			if( (ind_blocks[i] = get_block(disk_fd, data_freelist, data_start)) == -1) {
				fprintf(stderr, "Failed to get block from freelist\n");
				free(ind_blocks);
				return NULL;
			}

//printf("WRITE: indir, Writing blk: %i\n", ind_blocks[i]);

			// write the actual data block
			if( (*blocks_remaining) == 1 && f_size % BLOCK_SIZE ) {
				if( write_partial_block(disk_fd, ind_blocks[i], f_size % BLOCK_SIZE, data) ) {
					fprintf(stderr, "Failed to write file\n");
					free(ind_blocks);
					return NULL;
				}
			} else if(writeBlock(disk_fd, ind_blocks[i], (void *) &data[(num_blocks - *blocks_remaining)*BLOCK_SIZE]) == -1) {
				fprintf(stderr, "Failed to write to disk\n");
				free(ind_blocks);
				return NULL;
			}
			if( ((*blocks_remaining)--) < 1 ) { // decrement cause we have acutally written data
				break;
			}			
		}
		return ind_blocks; // return the list of indirection blocks
	 
	} else if(indir_lvl > 0) {

		// we still need some indirection, so allocate memory to hold the next level of indirection block numbers
		if( !(ind_list = (int *) calloc(BLOCK_SIZE/sizeof(int), sizeof(int))) ) {
			fprintf(stderr, "unable to allocate memory for indirection lv %i\n", indir_lvl);
			return NULL;
		}

		for(i = 0; i < BLOCK_SIZE/sizeof(int); i++){

			// get block for next indirection level
			if( (ind_list[i] = get_block(disk_fd, data_freelist, data_start)) == -1) {
				fprintf(stderr, "Failed to get block from freelist\n");
				return NULL;
			}

			// perform next indirection level
			if( !(ind_blocks = add_indir_data(blocks_remaining, f_size, num_blocks, data, disk_fd,
							 indir_lvl - 1, data_freelist, data_start)) ) {
				free(ind_list);
				return NULL;
			}
			// write the indirection level
			if(writeBlock(disk_fd, ind_list[i], (void*) ind_blocks) == -1) {
				free(ind_blocks);
				free(ind_list);
				fprintf(stderr, "Failed to write to disk\n");
				return NULL;	
			}
			free(ind_blocks);
			if( !(*blocks_remaining) ) { // dont decrement (no data written)
				break;
			}
		}
	  	return ind_list; // return the list of indirection blocks
	} else {
		fprintf(stderr, "invalid indir level %i\n", indir_lvl);
	}
	return NULL;
 }
 

// hand in a NULL inode if we need to create a new one
int write_file(inode node, fs_metadata metadata, char *data, int f_size) {
	int free_flag = 0;
	int i, ind_level, num_blocks, block, iters, blocks_remaining, disk_fd;
	int *indir_block;

	disk_fd = metadata->disk_fd;

	if( !node ) {
		free_flag = 1;
		node = (inode) malloc(sizeof(struct _inode));
		memset(node, 0x00, sizeof(struct _inode));
		node->num = get_inode(metadata->inode_freelist);
		node->type = FILE;
	}
	node->size = f_size;

	blocks_remaining = num_blocks = _ceiling(f_size, BLOCK_SIZE); // We need to change this when we put into global

	// write first ten blocks ()
	iters = (num_blocks < 11) ? num_blocks: 10;
	for(i = 0; i<iters; i++) {
		if( (block = get_block(disk_fd, metadata->data_freelist, metadata->data_start)) == -1 ) {
			fprintf(stderr, "Failed to get block\n");
			return -1;
		}
		node->block_list[i] = block;

//printf("WRITE: non-indir, Writing blk: %i\n", block);

		if(blocks_remaining == 1 && f_size % BLOCK_SIZE ) {
			if( write_partial_block(disk_fd, block, f_size % BLOCK_SIZE, data) ) {
				fprintf(stderr, "Failed to write file\n");
				return -1;
			}
		} else if( writeBlock(disk_fd, block, (void *) &data[i*BLOCK_SIZE]) == -1) {
			fprintf(stderr, "Failed to write file\n");
			return -1;
		}
		blocks_remaining--;
	}

	ind_level = 0;
 	while(blocks_remaining > 0){
 
 		if(ind_level >= 3){
 			fprintf(stderr, "Ye file be too big\n");
			return -1;
 		}
 
 		if( (block = get_block(disk_fd, metadata->data_freelist, metadata->data_start)) == -1 ) {
			fprintf(stderr, "Failed to get block\n");
			return -1;
		}	
 		node->block_list[i + ind_level] = block;

//printf("WRITE: indir block: %i\n", block);

 		if( !(indir_block = add_indir_data(&blocks_remaining, f_size, num_blocks, data, disk_fd, 
						ind_level, metadata->data_freelist, metadata->data_start)) ) {
			return -1;
		}
 		if(writeBlock(disk_fd, block, (void *) indir_block) == -1) {
			fprintf(stderr, "Failed to write indirection block\n");
			return -1;
		}
 		free(indir_block);

 		ind_level++;
 	}

	if(update_inode_block(disk_fd, node, metadata->inode_start) ) {
		fprintf(stderr, "failed to update inode block\n");
		return -1;
	}

	if(free_flag) {
		free(node);
	}
	return 0;
}


int update_directory(int disk_fd) {

	// read directory (read_file)

	// add or delete entry

	// write updated directory block

	return -1;
}



