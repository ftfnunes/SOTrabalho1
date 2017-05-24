#include "shutdown.h"
#include "utils.h"
#include <errno.h>

extern int errno;

int shm_pids = -1;
pids_t *pids;
int fila_shutdown = -1;
shutdown_vector_t msg;

/*
	shutdown.c

	Objetivo:
		Termina todos os processos. Caso tenha algum que ainda não foi executado informa quais não
		foram e a estatística dos já terminados.
*/

void init() {

	/* cria um novo segmento de memória compartilhada */
	shm_pids = shmget(MEM_PIDS, sizeof(pids_t), 0x1B6);
	if (shm_pids < 0) {
		printf("Erro na alocacao da memoria compartilhada\n");
		exit(1);
	}

	/* acopla o segmento de memória compartilhada identificado ao segmento de dados do processo que a chamou
	   e atribui o pid dos processos a serem encerrados. */
	pids = (pids_t*) shmat(shm_pids, 0, 0);
	if (pids < 0) {
		printf("Erro na associacao da memoria compartilhada\n");
		exit(1);
	}

	/* obtenção da fila de mensagens do shutdown */
	if((fila_shutdown = msgget(FILA_SHUTDOWN_K, IPC_CREAT | 0666)) < 0){
		printf("Erro na obtencao da fila de shutdown\n");
		exit(1);
	}
}

/* envia sinais SIGUSR1 para os processos, indicando a eles que devem terminar imediatamente.*/
void send_shutdown() {
	int i;

	for (i = 0; i < TAM_PIDS; ++i) {
		kill(pids->pids_v[i], SIGUSR1);
	}

	kill(pids->pid_esc, SIGUSR1);
}

/* impressão das estatística de cada processo que foi efetivamente executado */
int read_data() {
	int i, j;
	
	for (i = 0; i < TAM_PIDS; ++i) {
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

	/* destrói a fila de mensagens todas as outras operações em curso sobre essa fila 
	irão falhar e os processos bloqueados em espera de leitura ou escrita serão liberados. */
	msgctl(fila_shutdown, IPC_RMID, NULL);
	exit(0);
}