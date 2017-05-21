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
	char tempo_inicio[20];
	char tempo_fim[20];
	int turnaround;
};