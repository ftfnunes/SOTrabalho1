#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#define FILA_DO_ESCALONADOR_K 0x336
#define FILA_PARA_ESCALONADOR_K 0x337
#define TAM_PIDS 16
#define PID_ESCALONADOR 0
#define TAM_PROGRAMA 200
#define TAM_TEMPO 50
#define TAM_SHUTDOWN_V 15
#define SHUTDOWN_VECTOR_MTYPE 10

/*template para o envio de mensagens entre nos e escalonador*/
typedef struct {
	long mtype;
	struct msg_info {
		int node_dest;
		time_t tempo_submissao;
		char programa[TAM_PROGRAMA];
	} info;
} mensagem_exec_t;

typedef struct {
	long mtype;
	struct result_info {
		int node;
		char inicio[TAM_TEMPO];
		char fim[TAM_TEMPO];
		long turnaround;
	} info;
} resultado_t;

typedef struct {
	uint32_t pid;
	char programa[TAM_PROGRAMA];
	char tempo_inicio[TAM_TEMPO];
	char tempo_fim[TAM_TEMPO];
	char tempo_submissao[TAM_TEMPO];
} shutdown_data_t;

typedef struct {
	long mtype;
	struct vector_info{
		shutdown_data_t vetor[TAM_SHUTDOWN_V];
		uint8_t total;
	} info;
} shutdown_vector_t;

typedef struct {
	uint32_t pids_v[TAM_PIDS];
	uint8_t cont;
} pids_t;