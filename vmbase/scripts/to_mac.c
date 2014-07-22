#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
	int value = atoi(argv[1]);
	if(value < 16){
		printf("0%X", value);
	}
	else{
		printf("%X", value);
	}
	return EXIT_SUCCESS;
}
