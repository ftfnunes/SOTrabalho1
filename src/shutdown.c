#include "shutdown.h"
#include "utils.h"
#include <errno.h>

extern int errno;

int shm_pids = -1;
pids_t *pids;
int fila_shutdown = -1;
shutdown_vector_t msg;

void init() {
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
	if((fila_shutdown = msgget(FILA_SHUTDOWN_K, IPC_CREAT | 0666)) < 0){
		printf("Erro na obtencao da fila de shutdown\n");
		exit(1);
	}
}

void send_shutdown() {
	int i;
	for (i = 0; i < TAM_PIDS; ++i) {
		kill(pids->pids_v[i], SIGUSR1);
	}

	kill(pids->pid_esc, SIGUSR1);
}

int read_data() {
	int i, j;
	
	for (i = 0; i < 16; ++i) {
		if(msgrcv(fila_shutdown, &msg, sizeof(msg), 0, 0) < 0) {
			printf("Erro %d no recebimento da mensagem.\n", errno);
			exit(0);
		}

		printf("\n\n\t\tPrograma               PID     Tempo de Inicio            Tempo de Fim               Tempo de Submissao\n");
		for (j = 0; j < msg.info.total; ++j)	{
			printf("\t\t%-20s   %-5d   %-16s   %-16s   %-18s\n", msg.info.vetor[j].programa,
				msg.info.vetor[j].pid, msg.info.vetor[j].tempo_inicio, msg.info.vetor[j].tempo_fim, msg.info.vetor[j].tempo_submissao);
		}
		printf("\n\n");


	}
	return 0;
}

int main() {
	init();
	send_shutdown();
	read_data();
	msgctl(fila_shutdown, IPC_RMID, NULL);
	exit(0);
}