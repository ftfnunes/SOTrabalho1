#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct mensagem {
	long type;
	int node_dest;
	char programa[200];
};

struct resultado{
	long type;
	char inicio[50];
	char fim[50];
	long turnaround;
};