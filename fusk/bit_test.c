#include <string.h>
#include <stdio.h>
#include <stdlib.h>


void print_bits(char *bit_string) {
	int i, k, x, c = 8;
	
	printf("bitvector:\n");
	for(i=0; i<c; i++) {
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

int main(int argc, char *argv[]) {
	
	char input[20];
	char bit_string[8];
	char to_offset = 0x01;
	char offset;
	int bit_num, index_num;
	
	printf("\nq to exit\nenter bit position to flip!\n\n");
	
	memset(bit_string, 0xf0, 8*sizeof(char));
	print_bits(bit_string);

	while( fgets(input, 20, stdin) ) {
		if(input[0] == 'q') {
			break;
		}
		bit_num = atoi(input);
		
		index_num = bit_num / 8*sizeof(char);
		offset = to_offset << bit_num % 8*sizeof(char);
		printf("index %i: offset: %i\n", index_num, bit_num % 8*sizeof(char));
		
		bit_string[index_num] = bit_string[index_num] ^ offset;
		print_bits(bit_string);
	}
	
	return 0;
}




