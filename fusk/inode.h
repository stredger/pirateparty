#include <malloc.h>
#include "../fisk/p242pio.h"
#include "freelist.h"

#define BLOCK_SIZE 1024
#define INODE_BLKLIST_SIZE 13
#define MAX_DIR_ENTRIES 51
#define MAX_DIR_NAME_LEN 16
#define DIR 1
#define FILE 2

struct _inode {
	int num;
	char type;
	int size;
	int block_list[INODE_BLKLIST_SIZE];
};
typedef struct _inode *inode;


struct _dir { // size = 1024 bytes
	int entry_count;
	int inode_num[MAX_DIR_ENTRIES];
	char name[MAX_DIR_ENTRIES][MAX_DIR_NAME_LEN];
};
typedef struct _dir *dir;


typedef struct _inode_list_node *inode_list_node;
struct _inode_list_node{     
        int num;
       	inode_list_node next;
};


struct _inode_list{
        inode_list_node head;
        inode_list_node tail;
};
typedef struct _inode_list *inode_list;


/// this should be in a global thing
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
///////////////////////


int create_inode(int num, inode_list ilist);
int get_inode(inode_list ilist);
int update_inode_block(int disk_fd, inode node, int inode_block_start);
int fill_inode_list(int disk_fd, inode_list ilist, int first_inode_block, int inode_blocks);
inode_list create_inode_list(int disk_fd, int first_inode_block, int inode_blocks);
void destroy_inode_list(inode_list ilist);
void print_inode_list(inode_list);
char *read_file(fs_metadata metadata, inode node);
int write_file(inode node, fs_metadata metadata, char *data, int f_size);

