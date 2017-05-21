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

typedef struct {
	uint32_t pid;
	char programa[TAM_PROGRAMA];
	char tempo_inicio[TAM_TEMPO];
	char tempo_fim[TAM_TEMPO];
	char tempo_submissao[TAM_TEMPO];
} shutdown_data_t;