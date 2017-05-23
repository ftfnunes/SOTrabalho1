#include "shutdown.h"
#include "utils.h"

int shm_pids;
pids_t *pids;

void pids_init() {
	shm_pids = shmget(MEM_PIDS, sizeof(pids_t), 0x1B6);
	if (shm_pids < 0) {
		printf("Erro na alocacao da memoria compartilhada\n");
		exit(1);
	}
	pids = (pids_t*) shmat(shm_pids, 0, 0);
	if (pids < 0) {
		printf("Erro na associacao da memoria compartilhada\n");
		exit(1);
	}
}

void send_shutdown() {
	int i;
	for (i = 0; i < TAM_PIDS; ++i) {
		kill(pids->pids_v[i], SIGUSR1);
	}
}

void read_data() {
	shutdown_vector_t msg;
	int fila_shutdown, i, j;

	if((fila_shutdown = msgget(FILA_SHUTDOWN_K, 0666)) < 0){
		printf("Erro na obtencao da fila de shutdown\n");
		exit(1);
	}
	if(msgrcv(fila_shutdown, &msg, sizeof(shutdown_vector_t), SHUTDOWN_VECTOR_MTYPE, 0) < 0) {
		printf("Erro no recebimento da mensagem.\n");
	}
	for (i = 1; i < TAM_PIDS; ++i) {
		if(msgrcv(fila_shutdown, &msg, sizeof(shutdown_vector_t), SHUTDOWN_VECTOR_MTYPE, 0) < 0) {
			printf("Erro no recebimento da mensagem.\n");
		}
		printf("\t\tPrograma               PID     Tempo de Inicio    Tempo de Fim       Tempo de Submissao\n");
		for (j = 0; j < msg.info.total; ++j)	{
			printf("\t\t%-20s   %-5d   %-16s   %-16s   %-18s\n", msg.info.vetor[j].programa,
				msg.info.vetor[j].pid, msg.info.vetor[j].tempo_inicio, msg.info.vetor[j].tempo_fim, msg.info.vetor[j].tempo_submissao);
		}
	}
}

int main() {
	pids_init();
	send_shutdown();
	read_data();


	return 0;
}