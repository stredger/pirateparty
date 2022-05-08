#include "p242pio.h"
#include "p242pio.c"

int main ( int argc, char** argv){	
		char buff[1024];
	
		

		int fd;
		if((fd = openDisk("aaaaaa", 100*BLOCK_SIZE)) > 0){
			
			sprintf(buff, "this is sadsfasdfome text written at block 0\0", 1);
			writeBlock(fd, 0, (void*)buff);

		}


	
		if(readBlock(fd, 0, buff) > 0){
	
			printf ("Reading disk:\n%s\n\n", buff);
	
		}		
		if(readBlock(fd, 2, buff) > 0){
	
			printf ("Reading disk:\n%s\n\n", buff);
	
		}
	

	
	return 0;
}
