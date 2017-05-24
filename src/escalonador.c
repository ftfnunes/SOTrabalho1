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
#include <errno.h>

#include "utils.h"


/*
	escalonador.c

	Objetivo:
		Receber solicitações de execução de programas e gerenciá-las. Imprimir informações de execução.
*/

extern int errno;

int filaSolicitacoes = -1; 
int filaExecucao = -1;
int nodesToEsc = -1;

/* Foi decidido que o programa irá armazenar até 100 pids de filhos. */
int pids_procs[100];
int conta_procs = 0;
int shm_pids, idsem;
pids_t *pids;
struct sembuf operacao[2];

/* Estrutura que armazena a mensagem de solicitação recebida. */
mensagem_sol_t msg_sol;

/* Estrutura que armazena a mensagem a ser enviada para os gerentes de execução com o programa a ser executado. */
mensagem_exec_t msg_exe;


/* Rotina executada ao receber o sinal SIGUSR2, rotina que encerrará a execução dos filhos criados no fork dentro da função main. */
void trata_sigusr2(){
	printf("O programa '%s' não será executado devido ao término do processo.\n", msg_sol.info.programa);
	exit(0);
}

/* Função P do semáforo. */
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


/* Função V do semáforo. */
void v_sem(){
	/* Operação que retorna o semáforo a 0.*/
	operacao[0].sem_num = 0;
	operacao[0].sem_op = -1;
	operacao[0].sem_flg = 0;

	if(semop(idsem, operacao, 1) < 0)
		printf("Erro na operação V.\n");
}


/* Função que instancia um gerente de execução e retorna o pid do gerente. */
int instancia_gerente_de_execucao(int num_do_gerente){
	int pid;
	char num_gerente_string[3];

	sprintf(num_gerente_string, "%d", num_do_gerente);
	if((pid = fork()) == 0){
		execl("./gerente_de_execucao", "gerente_de_execucao", num_gerente_string, NULL);
	}

	return pid;
}


/* Instancia as filas de mensagem entre os gerentes de execução. */
int instancia_filas(){
	int i, id = -1;

	for(i=15; i>=0; i--){
		if((id = msgget(FILA_0_K+i, IPC_CREAT | 0666)) < 0){
			printf("Erro na criacao da fila 0x%x\n", FILA_0_K+i);
			exit(1);
		}
	}

	return id;
}


/* Essa função remove os recursos utilizados. */
void remove_recursos() {
	int i, id;

	/* Remove todas as filas de comunicação entre os processos gerentes de execução. */
	for(i = 0; i<16; i++){
		if((id = msgget(FILA_0_K+i, 0666)) < 0){
			printf("Erro na remocao da fila 0x%x\n", FILA_0_K+i);
			exit(1);
		}
		else{
			(msgctl(id, IPC_RMID, NULL) == 0) ? printf("Fila 0x%x removida\n", FILA_0_K+i) : printf("Erro, fila 0x%x nao pode ser removida\n", FILA_0_K+i);
		}
	}
	
	/* Remove a fila de mensagens de solicitação de execução. */
	(msgctl(filaSolicitacoes, IPC_RMID, NULL) == 0) ? printf("Fila de solicitacoes removida\n") : printf("Erro, fila de solicitacoes nao pode ser removida\n");

	/* Remove a fila de mensagem entre o gerente de execução 0 e o escalonador. */
	(msgctl(nodesToEsc, IPC_RMID, NULL) == 0) ? printf("Fila nodesToEsc removida\n") : printf("Erro, fila nodesToEsc nao pode ser removida %d\n", errno);

	/* Remove a área de memória compartilhada que armazena os pids do escalonador e do gerente de execução. */
	(shmctl(shm_pids, IPC_RMID, NULL) == 0) ? printf("Vetor de pids removido\n") : printf("Erro, vetor de pids nao pode ser removido\n");

	/* Remove o semáforo utilizado. */
	(semctl(idsem, 1, IPC_RMID, NULL) == 0) ?  printf("Semaforo removido\n") : printf("Erro, semaforo nao pode ser removido\n");
}


/* Finaliza o escalonador removendo recursos, realizando wait para os gerentes de execução e para os filhos do escalonador. */
void finaliza_escalonador(){
	int i, estado;

	remove_recursos();

	for(i=0; i<16; i++){
		wait(&estado);
	}

	/* Envia sinais SIGUSR2 para os filhos do escalonador, indicando a eles que devem terminar imediatamente.*/
	for (i = 0; i < conta_procs; ++i){
		kill(pids_procs[i], SIGUSR2);
	}
	/* Registrando o wait com o PID do filho antes de continuar o loop. */
	for(i = 0; i < conta_procs; i++){
		waitpid(pids_procs[i], &estado, 0);
	}
	exit(0);
}



int main(){
	int estado, pid = -1, job = 0, i = 0;

	resultado_t res;

	time_t tempo;

	/* Instrução que irá fazer com que o escalonador saia quando o sinal de término chegar. */
	signal(SIGUSR1, finaliza_escalonador);
	/* Instancia as filas entre os gerentes de execução dentro do Torus e retorna o id da fila para o no 0 */
	filaExecucao = instancia_filas();
	
	printf("Todas as filas do Torus criadas com sucesso.\n");

	/* Instancia uma área de memória compartilhada que abrigará os pids de todos os processos dos gerentes de execução. */

	/* Criação da fila de mensagens que recebe as solicitações de execução vindas dos processos de solicitação de execução. */
	if((filaSolicitacoes = msgget(FILA_SOLICITACAO_K, IPC_CREAT | 0666)) < 0){
		printf("Erro na criação da fila.\n");
		exit(1);
	}

	/* Criação da fila de mensagens para envio das mensagens co)m os programas a serem executados para os gerentes de execução. */

	if((nodesToEsc = msgget(FILA_PARA_ESCALONADOR_K, IPC_CREAT | 0666)) < 0) {
		printf("Erro na criação da fila.\n");
		exit(1); 
	}


	printf("Filas de solicitacao e de resultados criadas com sucesso!\n");

	shm_pids = shmget(MEM_PIDS, sizeof(pids_t), IPC_CREAT | 0666);
	if (shm_pids < 0) {
		printf("Erro na alocacao da memoria compartilhada\n");
		exit(1);
	}

	/* Atribuição à variável o ponteiro para a memória compartilhada. A área servirá para armazenar os pids dos gerentes de execução e do escalonador. */
	pids = (pids_t*) shmat(shm_pids, 0, 0);
	if (pids < 0) {
		printf("Erro na associacao da memoria compartilhada\n");
		exit(1);
	}

	/* Criação do semáforo. */
	if((idsem = semget(0x1010, 1, IPC_CREAT | 0666)) < 0){
		printf("Erro na criação de semáforo.\n");
		exit(1);
	}

	printf("Areas de memoria compartilhada (de pids e matriz de ocupacao) e semaforo criados com sucesso!\n");


	/* Instanciamento dos 16 gerentes de execução e armazenamento de seus pids. */
	printf("Pids dos gerentes: ");
	for(i = 0; i < 16; i++){
		pids->pids_v[i] = instancia_gerente_de_execucao(i);
		printf("%d\t", pids->pids_v[i]);
	}
	pids->pid_esc = getpid();

	printf("\nGerentes de execucao instanciados com sucesso!\n");

	/* Loop infinito do escalonador. */
	while(1){
		/* Recepção blocante de mensagem de solicitação de execução. */
		if(msgrcv(filaSolicitacoes, &msg_sol, sizeof(msg_sol), 0, 0) < 0){
			printf("Erro na recepcao de solicitacao no escalonador\n");
			exit(1);
		}

		/* Incremento do número do job ao chegar nova solicitação. */
		++job;

		/* Atribui à variável 'tempo' o tempo em que a mensagem foi recebida. */
		time(&tempo);

		/* Pai cria filho que tratará o dealy do programa, o envio do programa para os gerentes de execução e de recepção da mensagem de estatísticas ao final das execuções do programa. */
		pid = fork();

		if(pid == 0){

			/* Quando o filho recebe o sinal SIGUSR2, desvia o fluxo de execução para o encerramento do processo. */
			signal(SIGUSR2, trata_sigusr2);

			/* O processo irá realizar um sleep até que que delay desejado seja simulado. */
			sleep(msg_sol.info.seg);			

			/* Operação "P" de semáforo que garante que todos os gerentes de execução executarão somente um programa até que todos fiquem livres novamente. */
			p_sem();

			/* Serão enviadas 16 mensagens: 1 para cada gerente de execução. */
			for(i = 15; i >= 0; --i) {
				/* Atribui 1 ao tipo, pois esse tipo não é relevante nos processos gerenciadores de execução (não são verificados os tipos das mensagens) */
				msg_exe.mtype = 1;
				strcpy(msg_exe.info.programa, msg_sol.info.programa);
				msg_exe.info.node_dest = i;
				msg_exe.info.tempo_submissao = tempo;

				if(msgsnd(filaExecucao, &(msg_exe), sizeof(msg_exe), 0) < 0){
					printf("Erro no envio da mensagem do escalonador para o node %d\n", i);
					v_sem();
					exit(1);
				}
			}


			/* Após o envio de todas as solicitações, o filho espera as mensagens de estatísticas de execução de cada gerente ao final da execução do programa de forma blocante, garantindo que o loop só irá terminar após todos os gerentes finalizarem sua execução. Somente um programa será executado por todos, por vez. */
			for(i = 0; i < 16; ++i){
				if(msgrcv(nodesToEsc, &res, sizeof(res), 0, 0) < 0){
					printf("Erro na recepcao de resposta para o escalonador com erro %d\n", errno);
					v_sem();
					exit(1);
				}
				printf("job = %d, arquivo = %s, delay = %d, makespan = %ld\n", job, msg_sol.info.programa, msg_sol.info.seg, res.info.turnaround);
			}


			/* O semáforo é liberado e o processo é encerrado. */
			v_sem();			

			exit(0);
		}

		/* O escalonador armazena o pid de seu filho criado e incrementa o número de filhos que possui. */
		pids_procs[conta_procs] = pid;
		++conta_procs;

		
		/* É executado um loop de waitpid não blocantes que removem os filhos já encerrados. */
		for(i = 0; i < conta_procs; i++){
			waitpid(pids_procs[i], &estado, WNOHANG);
		}
		
	}

	return 0;
}