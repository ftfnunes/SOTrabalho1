
#include "shutdown.h"
#include "utils.h"

int shutdown_mutex;
int shm_pids;
pids_t *pids;

void mutex_init() {
	shutdown_mutex = semget(0x120700, 1, 0x1B6); 
	if (shutdown_mutex < 0) {
		printf("Erro na criacao do semaforo.\n");
		exit(1);
	}
}

void pids_init() {
	shm_pids = shmget(0x120700, sizeof(pids_t), 0x1B6);
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
	for (i = 0; i < pids->cont; ++i) {
		kill(pids->pids_v[i], SIGUSR1);
	}
}

void read_data() {
	shutdown_vector_t msg;
	int i, j;
	for (i = 1; i < pids->cont; ++i) {
		if(msgrcv(0x120700, &msg, sizeof(shutdown_vector_t), SHUTDOWN_VECTOR_MTYPE, 0) < 0) {
			printf("Erro no recebimento da mensagem.\n");
		}
		printf("\t\tPrograma               PID     Tempo de Inicio    Tempo de Fim       Tempo de Submissao\n");
		for (j = 0; j < msg.total; ++j)	{
			printf("\t\t%-20s   %-5d   %-16s   %-16s   %-18s\n", msg.vetor[j].programa,
				msg.vetor[j].pid, msg.vetor[j].tempo_inicio, msg.vetor[j].tempo_fim, msg.vetor[j].tempo_submissao);
		}
	}
}

int main() {
	mutex_init();
	pids_init();
	send_shutdown();
	read_data();


	return 0;
}