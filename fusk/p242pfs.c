
#include "../fisk/p242pio.h"

#include "inode.h"


#define BLOCK_SIZE 1024
#define FS_TYPE "pier2pier\0"
#define FS_TYPE_LEN 10
#define SUPERBLOCK 0
#define BYTE_SIZE 8


struct _superblock {
	char fs_type[FS_TYPE_LEN];
	int inode_start;
	int data_start;
	int total_blocks;
	int cur_freelist_char;
};
typedef struct _superblock *superblock;

/*
struct _fs_metadata {
	int disk_fd;
	int inode_start;
	int data_start;
	int total_blocks;
	freelist data_freelist;
	inode_list inode_freelist;
	int cur_freelist_char;
};
typedef struct _fs_metadata *fs_metadata;
*/

// Calculate the ceiling of X / Y
int ceiling (int x, int y) {
	return (x % y) ? x / y + 1 : x / y;
}


int init_superblock(fs_metadata metadata, int disk_fd, 
		int freelist_blocks, int inode_blocks, int data_blocks) {
	int bytes_written;
	char buff[BLOCK_SIZE];
	superblock sblock;

//	printf("Initializing superblock\n");
	
	memset(buff, 0x00, BLOCK_SIZE*sizeof(char));
	sblock = (superblock) buff;
	memcpy(sblock->fs_type, FS_TYPE, FS_TYPE_LEN*sizeof(char));
	
	metadata->inode_start = sblock->inode_start = 1 + freelist_blocks;
	metadata->data_start = sblock->data_start = sblock->inode_start + inode_blocks;
	metadata->total_blocks = sblock->total_blocks = sblock->data_start + data_blocks;
	
	if((bytes_written = writeBlock(disk_fd, SUPERBLOCK, (void*)buff)) == -1) {
		fprintf(stderr, "Failed to write to disk %i\n", disk_fd);
		return -1;
	}
	return 0;
}


int write_blank_blocks(int disk, int n, int num) {
	char buff[BLOCK_SIZE];
	
	memset(buff, 0x00, BLOCK_SIZE*sizeof(char));

	for( ;n<=num; n++) {
		if(writeBlock(disk, n, (void *)buff) == -1) {
			fprintf(stderr, "Failed to write to disk %i\n", disk);
			return -1;
		}
	}
	return 0;
}


// Initialize the disk, called once when user asks to create a disk
int initialize_disk(fs_metadata metadata, int disk_fd, 
		int freelist_blocks, int inode_blocks, int data_blocks) {
	char buffer[BLOCK_SIZE], buff2[BLOCK_SIZE];
	inode root_node;
	dir root_dir;

	if( init_superblock(metadata, disk_fd, freelist_blocks, inode_blocks, data_blocks) == -1 ) {
		return -1;
	}

	// clear freelist
	if(write_blank_blocks(disk_fd, 1, freelist_blocks)) {
		return -1;
	}
	// init freelist
	metadata->data_freelist = create_new_freelist(disk_fd, freelist_blocks, metadata->cur_freelist_char, metadata->data_start);

	// inode freelist
	metadata->inode_freelist = create_inode_list(disk_fd, metadata->inode_start, inode_blocks);


	// create root dir and associated inode
	memset(buffer, 0x00, BLOCK_SIZE*sizeof(char));
	root_node = (inode)buffer;
	root_node->num = get_inode(metadata->inode_freelist);
	root_node->type = DIR;
	root_node->size = BLOCK_SIZE;
	root_node->block_list[0] = get_block(disk_fd, metadata->data_freelist, metadata->data_start);
	update_inode_block(disk_fd, root_node, metadata->inode_start);

//printf("init after root: 1st free block: %i\n", metadata->data_freelist->head->block);
	// update dir
	memset(buff2, 0x00, BLOCK_SIZE*sizeof(char));
	root_dir = (dir)buff2;
	root_dir->inode_num[0] = 0;
	memcpy(root_dir->name[0], ".\0", 2*sizeof(char));

//printf("Writing block %i\n", root_node->block_list[0]);

	writeBlock(disk_fd, root_node->block_list[0], buff2);
	// end update dir
	
	return 0;
} 


int get_disk_metadata(int disk_fd, superblock sblock, fs_metadata metadata) {

	metadata->inode_start = sblock->inode_start;
	metadata->data_start = sblock->data_start;
	metadata->total_blocks = sblock->total_blocks;
	metadata->cur_freelist_char = sblock->cur_freelist_char;

	// inode start - 1 = number of freelist blocks
	metadata->data_freelist = create_new_freelist(disk_fd, metadata->inode_start - 1, metadata->cur_freelist_char, metadata->data_start);
	
	// data_start - inode_start = # data blocks
	metadata->inode_freelist = create_inode_list(disk_fd, metadata->inode_start, metadata->data_start - metadata->inode_start);	

	
//	printf("inode start: %i\n", metadata->inode_start);
//	printf("data start: %i\n", metadata->data_start);
//	printf("1st free block: %i\n", metadata->data_freelist->head->block);
//	printf("tot blocks: %i\n", metadata->total_blocks);
//	printf("freelist char: %i\n", metadata->cur_freelist_char);


	return 0;
}


// Startup routine, called every time the program starts
int startup_disk(fs_metadata metadata, char *disk_name, int disk_size) {
	int disk_fd, data_blocks, bytes_read, inode_blocks, freelist_blocks, adjusted_disk_blocks;
	char buff[BLOCK_SIZE];
	
	// calculate required blocks for metadata of the filesystem
	data_blocks = ceiling(disk_size, BLOCK_SIZE);
	freelist_blocks = ceiling(data_blocks, BLOCK_SIZE*BYTE_SIZE);
	inode_blocks = data_blocks / (BLOCK_SIZE / sizeof(struct _inode)); // we want the floor here	
	adjusted_disk_blocks = data_blocks + inode_blocks + freelist_blocks + 1; // +1 for Superblock

	// print out specs for debuging!
//	printf("Disk Specs\n");
//	printf("data blocks: %i\n", data_blocks);
//	printf("freelist blocks: %i\n", freelist_blocks);
//	printf("inode size: %li\n", sizeof(struct _inode));
//	printf("inodes/block: %li\n", BLOCK_SIZE / sizeof(struct _inode));
//	printf("inode blocks: %i\n", inode_blocks);
//	printf("dir size: %li\n", sizeof(struct _dir));
//	printf("adj size: %i\n\n", adjusted_disk_blocks);
	
	if( (disk_fd = openDisk(disk_name, adjusted_disk_blocks*BLOCK_SIZE)) == -1 ) {
		fprintf(stderr, "Failed to open disk %s\n", disk_name);
		return -1;
	}
	metadata->disk_fd = disk_fd;

	if( !(bytes_read = readBlock(disk_fd, SUPERBLOCK, buff)) ) {
		if(initialize_disk(metadata, disk_fd, freelist_blocks, inode_blocks, data_blocks)) {
			return -1;
		}
	} else if(bytes_read == -1) {
		fprintf(stderr, "Failed to read from disk %s\n", disk_name);
		return -1;
	} else if(strcmp( ((superblock)buff)->fs_type, FS_TYPE) ){
		fprintf(stderr, "%s is not a pier2pier file!! YAAAR\n", disk_name);
		return -1;	
	} else if(get_disk_metadata(disk_fd, (superblock)buff, metadata )) {
		fprintf(stderr, "Failed to read metadata from %s\n", disk_name);
		return -1;
	}
	return 0;
}


fs_metadata init_metadata() {
	fs_metadata metadata;

	if( !(metadata = (fs_metadata) malloc(sizeof(struct _fs_metadata))) ) {
		fprintf(stderr, "Failed to allocate memory for metadata\n");
		return NULL;
	}
	memset(metadata, 0x00, sizeof(struct _fs_metadata));

	return metadata;
}


int main (int argc, char *argv[]) {
	char *diskname = "testdisk\0";
	fs_metadata metadata = NULL;

	if( !(metadata = init_metadata()) ) {
		return -1;
	}
	if(startup_disk(metadata, diskname, 100000*BLOCK_SIZE)) {
		free(metadata);
		return -1;
	}
 
//	get_block(metadata->disk_fd, metadata->data_freelist, metadata->data_start);
//	get_inode(metadata->inode_freelist);

	struct _inode n;
printf("pre Testing!\n");
fflush(stdout);
	
	char *file_data, *f;
	n.num = get_inode(metadata->inode_freelist);
	n.type = FILE;

	file_data = (char *) malloc(10000*BLOCK_SIZE);

printf("Testing!\n");
fflush(stdout);

	memset(file_data, 0x00, 10000*BLOCK_SIZE*sizeof(char));
	memcpy(file_data, "Fucking sweet!", 7*sizeof(char));

	if(write_file(&n, metadata, file_data, 10000*BLOCK_SIZE) == -1) {
		printf("didnt write...\n");
	}

printf("Also Testing!\n");
fflush(stdout);
	if( !(f = read_file(metadata, &n) )) {
		printf("didnt read...\n");
	} else {
		printf("%s\n", f);
	}
	free(f);

	printf("\ndata freelist\n");
	printf("head: %i\n", metadata->data_freelist->head->block);
//	print_freelist(metadata->data_freelist);
	printf("\ninode list\n");
	printf("head: %i\n", metadata->inode_freelist->head->num);
//	print_inode_list(metadata->inode_freelist);

	destroy_freelist(metadata->data_freelist);
	destroy_inode_list(metadata->inode_freelist);
	free(metadata);
	return 0;
}
