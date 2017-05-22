#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/signal.h>
#include <sys/sem.h>

#include "utils.h"


/*
	Mecanismo para escalonar os processos a serem executados.

	Key para obtenção da fila de mensagens: A_DEFINIR .

	É necessário definir a estrutura de dados que irá estar na mensagem recebida pelo gerenciador de solicitação de execução:

	struct solicitacao{
		long pid;
		//Estrutura da mensagem
	}

	Também é necessário definir a estrutura a ser enviada para os gerenciadores de execução
*/


int filaSolicitacoes = -1; 
int filaExecucao = -1;
int shmid;
uint8_t  *mtzGerentesExec; 
int shm_pids, nodesToEsc, idsem;
pids_t *pids;
struct sembuf operacao[2];

void p_sem(){
	/* Operação que verifica se o semáforo é igual a 0.*/
	operacao[0].sem_num = 0;
	operacao[0].sem_op = 0;
	operacao[0].sem_flg = 0;
	/* Operação que adiciona 1 do semáforo. */
	operacao[1].sem_num = 0;
	operacao[1].sem_op = 1;
	operacao[1].sem_flg = 0;

	if(semop(idsem, operacao, 2) < 0)
		printf("Erro na operação P.\n");
}

void v_sem(){
	/* Operação que retorna o semáforo a 0.*/
	operacao[0].sem_num = 0;
	operacao[0].sem_op = -1;
	operacao[0].sem_flg = 0;

	if(semop(idsem, operacao, 1) < 0)
		printf("Erro na operação V.\n");
}



int instancia_gerente_de_execucao(int num_do_gerente){
	int pid;
	char num_gerente_string[3];

	sprintf(num_gerente_string, "%d", num_do_gerente);
	if((pid = fork()) == 0){
		execl("./gerente_de_execucao", "gerente_de_execucao", num_gerente_string, NULL);
	}

	return pid;
}

void instancia_filas(){
	int i;

	for(i = 0; i<=11; i++){
		if(msgget((i*10)+i+4, IPC_CREAT | 0666) < 0){
			printf("Erro na criacao da fila %d\n", (i*10)+i+4);
			exit(1);
		}

		if(i < 3){
			if(msgget((i*10)+i+1, IPC_CREAT | 0666) < 0){
				printf("Erro na criacao da fila %d\n", (i*10)+i+1);
				exit(1);
			}
		}
	}
}

void remove_recursos() {
	int i, id;

	for(i = 0; i<=11; i++){
		if((id = msgget((i*10)+i+4, 0666)) < 0){
			printf("Erro na remocao da fila %d\n", (i*10)+i+4);
			exit(1);
		}

		else{
			(msgctl(id, IPC_RMID, NULL) == 0) ? printf("Fila %d removida\n", (i*10)+i+4) : printf("Erro, fila %d nao pode ser removida\n", (i*10)+i+4);
		}

		if(i < 3){
			if((id = msgget((i*10)+i+1, 0666)) < 0){
				printf("Erro na remocao da fila %d\n", (i*10)+i+1);
				exit(1);
			}
			else{
				(msgctl(id, IPC_RMID, NULL) == 0) ? printf("Fila %d removida\n", (i*10)+i+1) : printf("Erro, fila %d nao pode ser removida\n", (i*10)+i+1);
			}
		}
	}

	(msgctl(filaExecucao, IPC_RMID, NULL) == 0) ? printf("Fila de execucao removida\n") : printf("Erro, fila de execucao nao pode ser removida\n");
	(msgctl(filaSolicitacoes, IPC_RMID, NULL) == 0) ? printf("Fila de solicitacoes removida\n") : printf("Erro, fila de solicitacoes nao pode ser removida\n");
	(msgctl(nodesToEsc, IPC_RMID, NULL) == 0) ? printf("Fila nodesToEsc removida\n") : printf("Erro, fila nodesToEsc nao pode ser removida\n");

	(shmctl(shmid, IPC_RMID, NULL) == 0) ? printf("Matriz de ocupacao removida\n") : printf("Erro, matriz de ocupacao nao pode ser removida\n");
	(shmctl(shm_pids, IPC_RMID, NULL) == 0) ? printf("Vetor de pids removido\n") : printf("Erro, vetor de pids nao pode ser removido\n");

	(semctl(idsem, 1, IPC_RMID, NULL) == 0) ?  printf("Semaforo removido\n") : printf("Erro, semaforo nao pode ser removido\n");
}


void finaliza_escalonador(){
	int i, estado;
	remove_recursos();
	for(i=0; i<16; i++){
		wait(&estado);
	}		
	exit(0);
}



int main(){
	int estado, pid = -1, job = 0, i = 0;

	mensagem_sol_t msg_sol;
	mensagem_exec_t msg_exe;
	resultado_t res;

	time_t tempo;

	/* Instrução que irá fazer com que o escalonador saia quando o sinal de término chegar. */
	signal(SIGUSR1, finaliza_escalonador);

	/* Instancia as filas entre os gerentes de execução dentro do Torus. */
	instancia_filas();
	printf("Todas as filas do Torus criadas com sucesso.\n");

	/* Instancia uma área de memória compartilhada que abrigará os pids de todos os processos dos gerentes de execução. */

	/* Criação da fila de mensagens que recebe as solicitações de execução vindas dos processos de solicitação de execução. */
	if((filaSolicitacoes = msgget(FILA_SOLICITACAO_K, IPC_CREAT | 0666)) < 0){
		printf("Erro na criação da fila.\n");
		exit(1);
	}

	/* Criação da fila de mensagens para envio das mensagens co)m os programas a serem executados para os gerentes de execução. */
	if((filaExecucao = msgget(FILA_DO_ESCALONADOR_K, IPC_CREAT | 0666)) < 0){
		printf("Erro na criação da fila.\n");
		exit(1); 
	}

	if((nodesToEsc = msgget(FILA_PARA_ESCALONADOR_K, IPC_CREAT | 0666)) < 0) {
		printf("Erro na criação da fila.\n");
		exit(1); 
	}


	printf("Filas de solicitacao, de execucao e nodesToEsc criadas com sucesso!\n");

	shm_pids = shmget(MEM_PIDS, sizeof(pids_t), IPC_CREAT | 0666);
	if (shm_pids < 0) {
		printf("Erro na alocacao da memoria compartilhada\n");
		exit(1);
	}
	/* Atribuição à variável o ponteiro para a memória compartilhada. */
	pids = (pids_t*) shmat(shm_pids, 0, 0);
	if (pids < 0) {
		printf("Erro na associacao da memoria compartilhada\n");
		exit(1);
	}

	/* Criação de um vetor de inteiros em memória compartilhada para verificar se os gerentes de execução estão livres. */
	if((shmid = shmget(0x33, 16*sizeof(uint8_t), IPC_CREAT | 0666)) < 0){
		printf("Erro na criação de memória compartilhada.\n");
		exit(1);
	}

	/* Atribuição à variável "mtzGerentesExec" a área de memória compartilhada que abriga o vetor com o status dos gerentes de execução, se estão livre ou nao. */
	if((mtzGerentesExec = (uint8_t *) shmat(shmid, 0, 0)) < 0){
		printf("Erro na atribuição de memória compartilhada.\n");
		exit(1);
	}

	for(i = 0; i < 16; i++){
		mtzGerentesExec[i] = 0;
	}

	/* Criação do semáforo. */
	if((idsem = semget(0x1010, 1, IPC_CREAT | 0666)) < 0){
		printf("Erro na criação de semáforo.\n");
		exit(1);
	}

	printf("Areas de memoria compartilhada (de pids e matriz de ocupacao) e semaforo criados com sucesso!\n");


	printf("Pids dos gerentes: ");
	for(i = 0; i < 16; ++i){
		pids->pids_v[i] = instancia_gerente_de_execucao(i);
		printf("%d\t", pids->pids_v[i]);
	}
	pids->pid_esc = getpid();

	printf("\nGerentes de execucao instanciados com sucesso!\n");

	while(1){
		if(msgrcv(filaSolicitacoes, &msg_sol, sizeof(msg_sol), 0, 0) < 0){
			printf("Erro na recepcao de solicitacao no escalonador\n");
			exit(1);
		}
		
		++job;

		/* Atribui à variável 'tempo' o tempo em que a mensagem foi recebida. */
		time(&tempo);

		pid = fork();

		if(pid == 0){
			/* O processo irá realizar um sleep até que que delay desejado seja simulado. */
			sleep(msg_sol.info.seg);			/* Loop verifica se todos os gerenciadores de processo estão livres.*/

			p_sem();

			for(i = 15; i >= 0; --i) {
				/* Atribui 1 ao tipo, pois esse tipo não é relevante nos processos gerenciadores de execução (não são verificados os tipos das mensagens) */
				msg_exe.mtype = 1;
				strcpy(msg_exe.info.programa, msg_sol.info.programa);
				msg_exe.info.node_dest = i;
				msg_exe.info.tempo_submissao = tempo;

				if(msgsnd(filaExecucao, &(msg_exe), sizeof(msg_exe), 0) < 0){
					printf("Erro no envio da mensagem do escalonador para o node %d\n", i);
					exit(1);
				}
			}

			for(i = 0; i < 16; ++i){
				if(msgrcv(nodesToEsc, &res, sizeof(res), 0, 0) < 0){
					printf("Erro na recepcao de resposta para o escalonador\n");\
					exit(1);
				}
				printf("job = %d, arquivo = %s, delay = %d, makespan = %ld\n", job, msg_sol.info.programa, msg_sol.info.seg, res.info.turnaround);
			}

			v_sem();			

			exit(0);
		}

		/* Registrando o wait com o PID do filho antes de continuar o loop. */
		waitpid(pid, &estado, WNOHANG);
	}

	return 0;
}