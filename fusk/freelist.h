typedef struct _freelist_node *freelist_node;
typedef struct _freelist *freelist;

struct _freelist{
        
        freelist_node head;
        freelist_node tail;
        int num_freelists;//number of blocks of the freelist
        int cur_char;
        int count;
        
};

struct _freelist_node{
        
        int block;
        freelist_node next;
        
};


void print_freelist(freelist l);
void sync_freelist(int disk_fd, int block_num, int data_block_start);
int get_block(int disk_fd, freelist fl, int data_block_start);
int add_block(int disk_fd, freelist fl, int block_num, int data_block_start);
int populate_freelist(int disk_fd, freelist fl, int initial_char, int data_block_start);
freelist create_new_freelist(int disk_fd, int freelist_blocks, int cur_char, int data_block_start);
void destroy_freelist(freelist l);
