#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define FILA_DO_ESCALONADOR_K 0X334
#define FILA_PARA_ESCALONADOR_K 0X335

struct mensagem_exe {
	long type;
	int node_dest;
	time_t tempo_submissao;
	char programa[200];
};

struct resultado{
	long type;
	int node;
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

typedef struct {
	long mtype;
	shutdown_data_t vetor[TAM_SHUTDOWN_V];
	uint8_t total;
} shutdown_vector_t;