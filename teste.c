#include <stdio.h>

int main(){
	long int resultado, i;

	for(i=1; i<100; i++){
		resultado = i*i;
		printf("%ld\n", resultado);
	}

	printf("\n\n\n");
	return 0;
}