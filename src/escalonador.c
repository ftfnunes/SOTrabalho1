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


int filaSolicitacoes, filaExecucao,  *mtzGerentesExec, shmid;
int shm_pids, nodesToEsc;
pids_t *pids;



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

void remove_filas(){
	int i, id;

	for(i = 0; i<=11; i++){
		if((id = msgget((i*10)+i+4, 0666)) < 0){
			printf("Erro na remocao da fila %d\n", (i*10)+i+4);
			exit(1);
		}

		msgctl(id, IPC_RMID, NULL);

		if(i < 3){
			if((id = msgget((i*10)+i+1, 0666)) < 0){
				printf("Erro na remocao da fila %d\n", (i*10)+i+1);
				exit(1);
			}
			msgctl(id, IPC_RMID, NULL);
		}
	}

	msgctl(filaExecucao, IPC_RMID, NULL);
	msgctl(filaSolicitacao, IPC_RMID, NULL);
	msgctl(nodesToEsc, IPC_RMID, NULL);

	shmctl(shmid, IPC_RMID, NULL);
	shmctl(shm_pids, IPC_RMID, NULL);


}


void finaliza_escalonador(){
	remove_filas();		
	exit(0);
}



int main(){
	int estado, pid;

	struct mensagem_sol msg_sol;
	struct mensagem_exe msg_exe[16];
	int i, sai = 0, conta_livres;
	time_t tempo;

	/* Instrução que irá fazer com que o escalonador saia quando o sinal de término chegar. */
	signal(SIGUSR1, finaliza_escalonador);


	instancia_filas();

	shm_pids = shmget(0x120700, sizeof(pids_t), IPC_CREAT | 0666);
	if (shm_pids < 0) {
		printf("Erro na alocacao da memoria compartilhada\n");
		exit(1);
	}
	pids = (pids_t*) shmat(shm_pids, 0, 0);
	if (pids < 0) {
		printf("Erro na associacao da memoria compartilhada\n");
		exit(1);
	}

	for(i = 0; i < 16; ++i){
		pids[i] = instancia_gerente_de_execucao(i);
	}

	i = 0;



	/* Criação da fila de mensagens que recebe as solicitações de execução vindas dos processos de solicitação de execução. */
	if(filaSolicitacoes = msgget(0x1, IPC_CREAT | 0666) < 0){
		printf("Erro na criação da fila.\n");
		exit(1);
	}

	/* Criação da fila de mensagens para envio das mensagens com os programas a serem executados para os gerentes de execução. */
	if(filaExecucao = msgget(FILA_DO_ESCALONADOR_K, IPC_CREAT | 0666) < 0){
		printf("Erro na criação da fila.\n");
		exit(1); 
	}

	/* Criação de um vetor de inteiros em memória compartilhada para verificar se os gerentes de execução estão livres. */
	if(shmid = shmget(0x33, 16*sizeof(uint8_t), IPC_CREAT | 0666)){
		printf("Erro na criação de memória compartilhada.\n");
		exit(1);
	}

	/* Atribuição à variável "mtzGerentesExec" a área de memória compartilhada que abriga o vetor com o status dos gerentes de execução, se estão livre ou nao. */
	if(mtzGerentesExec = (int *) shmat(shmid, 0, 0)){
		printf("Erro na atribuição de memória compartilhada.\n");
		exit(1);
	}

	while(1){
		msgrcv(filaSolicitacoes, &msg_sol, sizeof(msg_sol), 0, 0);

		/* Atribui à variável 'tempo' o tempo em que a mensagem foi recebida. */
		time(&tempo);

		pid = fork();

		if(pid == 0){
			/* O processo irá realizar um sleep até que que delay desejado seja simulado. */
			sleep(msg_sol.sol.seg);

			/* Loop verifica se todos os gerenciadores de processo estão livres.*/
			while(!sai){
				conta_livres = 0;
				for(i = 0; i < 16; ++i){
					if(mtzGerentesExec[i] == 0 )
						++conta_livres;
				}
				if(conta_livres == 16)
					sai = 1;
			}

			for(i = 15; i >= 0; --i) {
				/* Atribui 1 ao tipo, pois esse tipo não é relevante nos processos gerenciadores de execução (não são verificados os tipos das mensagens) */
				msg_exe[i].type = 1;
				strcpy(msg_exe[i].info.programa, msg_sol.sol.programa);
				msg_exe[i].info.node_dest = i;
				msg_exe[i].info.tempo_submissao = tempo;

				msgsnd(filaExecucao, &(msg_exe[i]), sizeof(struct mensagem_exe), IPC_NOWAIT);
			}

			/*Não estamos verificando se a fila de mensagens está cheia. */
			

			exit(0);
		}

		/* Registrando o wait com o PID do filho antes de continuar o loop. */
		waitpid(pid, &estado, WNOWAIT);
	}

	return 0;
}