#include "p242pio.h"
#include "p242pio.c"
#define MAX_RTHREADS 10
#define MAX_WTHREADS 10


struct write_t{
	int fd;
	int t_id;
};typedef struct write_t w_ctx;

struct read_t{
	int fd;
	int t_id;
};typedef struct read_t r_ctx;
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;

void *readThread(void *rctx_ptr){
	void *readBuffer = (void*)calloc(10, 1024);
	int rd;
   r_ctx *rctx = (r_ctx*)rctx_ptr;	
	pthread_mutex_lock(&file_lock);
	if((rd = readBlock(rctx->fd, rctx->t_id, readBuffer)) >= 0){
		printf("Read Thread %d wrote to block %d, data read: %s\n", rctx->t_id, rctx->t_id, (char*)readBuffer);
	}	
	pthread_mutex_unlock(&file_lock);
	free(readBuffer);
	pthread_exit(NULL);
}
void *writeThread(void *wctx_ptr){
	int wt;
	char strbuff[50];  
	pthread_mutex_lock(&file_lock);
	w_ctx *wctx = (w_ctx*)wctx_ptr;
	sprintf(strbuff, "Write by thread %d", wctx->t_id);
	if((wt = writeBlock(wctx->fd, wctx->t_id, (void*)strbuff)) >= 0){
		printf("Thread %d wrote to block %d, data written: %s\n", wctx->t_id, wctx->t_id, (char*)strbuff);		
	}
	else{
		printf("Write failed\n");
	}
	pthread_mutex_unlock(&file_lock);
	pthread_exit(NULL);
}
int main(void){
	pthread_t rthreads[MAX_RTHREADS];
	pthread_t wthreads[MAX_WTHREADS];
	int fd = -1;
	void *writeStatus, *readStatus;
	int writeError, readError;
	int i;
   w_ctx wctx[MAX_WTHREADS];
	r_ctx rctx[MAX_RTHREADS];
	if((fd = openDisk("diskfile", 100*BLOCK_SIZE)) > 0){
		for(i = 0; i < MAX_RTHREADS; i++){
			
			wctx[i].fd = fd;
			wctx[i].t_id = i;
			writeError = pthread_create(&wthreads[i], NULL, writeThread, &wctx[i]);
			if(writeError){
				printf("Error in wthread %d.\n", i);
			}	
		}
		for(i = 0; i<MAX_RTHREADS; i++){
			pthread_join(wthreads[i], &writeStatus);
		}
		for(i = 0; i< MAX_WTHREADS; i++){
			rctx[i].fd = fd;
			rctx[i].t_id = i;
			readError = pthread_create(&rthreads[i], NULL, readThread, &rctx[i]);
			if(readError){
				printf("Error in rthread %d.\n", i);
			}

		}
		for(i = 0; i<MAX_RTHREADS; i++){
			pthread_join(rthreads[i], &readStatus);
		}
	}
		
	pthread_exit(NULL);
}

