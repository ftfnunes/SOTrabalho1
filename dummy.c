#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include "utils.h"

#define TIPO_MSG_PIDS 1

extern int errno;

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
}

void teste_gerente(){
	int escToNode, nodesToEsc;
	int i;
	int pids[16];
	mensagem_exec_t msg;
	resultado_t rst;
	shutdown_vector_t estats;
	int fila_sh;


	instancia_filas();

	escToNode = msgget(FILA_0_K, IPC_CREAT | 0666);

	nodesToEsc = msgget(FILA_PARA_ESCALONADOR_K, IPC_CREAT | 0666);

	fila_sh = msgget(0x120700, IPC_CREAT | 0666);

	for(i=0; i<16; i++){
		pids[i] = instancia_gerente_de_execucao(i);
	}

	msg.info.node_dest = 15;
	msg.mtype = 1;
	time(&(msg.info.tempo_submissao));
	strcpy(msg.info.programa, "./teste");


	if(msgsnd(escToNode, &msg, sizeof(msg), 0) < 0){
		printf("Erro %d\n", errno);
	}

	printf("mensagem enviada\n");


	if(msgrcv(nodesToEsc, &rst, sizeof(rst), 0, 0) < 0){
		printf("Erro %d\n", errno);
	}

	printf("Mensagem Recebida %d %s %s %ld\n", rst.info.node, rst.info.inicio, rst.info.fim, rst.info.turnaround);


	kill(pids[15], SIGUSR1);

	if(msgrcv(fila_sh, &estats, sizeof(estats), 0, 0) < 0){
		printf("Erro %d\n", errno);
	}

	printf("%s %s %s %s %d\n", estats.info.vetor[0].programa, estats.info.vetor[0].tempo_fim, estats.info.vetor[0].tempo_inicio, estats.info.vetor[0].tempo_submissao, estats.info.vetor[0].pid );

	sleep(10);

	for(i=0; i<15;i++){
		kill(pids[i], SIGKILL);
	}

	remove_filas();
	msgctl(escToNode, IPC_RMID, NULL);
	msgctl(nodesToEsc, IPC_RMID, NULL);
	msgctl(fila_sh, IPC_RMID, NULL);
}


int main(){
	int filaSol, i, j, shm_pids;
	pids_t *pids;
	mensagem_sol_t msg;
	shutdown_vector_t estatisticas;
	int fila_sh = -1;


	fila_sh = msgget(FILA_SHUTDOWN_K, IPC_CREAT | 0666);

	shm_pids = shmget(MEM_PIDS, sizeof(pids_t), 0666);
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

	if((filaSol = msgget(FILA_SOLICITACAO_K, 0666)) < 0){
		printf("Erro na criação da fila.\n");
		exit(1);
	}

	msg.mtype = 1;
	strcpy(msg.info.programa, "./teste");
	msg.info.seg = 3;

	printf("solicitacao enviada %d %s\n", msg.info.seg, msg.info.programa);

	if(msgsnd(filaSol, &msg, sizeof(msg), 0) < 0){
		printf("Erro no envio %d\n", errno);
		exit(1);
	}

	sleep(2);

	msg.mtype = 1;
	strcpy(msg.info.programa, "./teste");
	msg.info.seg = 3;


	printf("solicitacao enviada %d %s\n", msg.info.seg, msg.info.programa);

	if(msgsnd(filaSol, &msg, sizeof(msg), 0) < 0){
		printf("Erro no envio %d\n", errno);
		exit(1);
	}

	msg.mtype = 1;
	strcpy(msg.info.programa, "./teste");
	msg.info.seg = 6;

	printf("solicitacao enviada %d %s\n", msg.info.seg, msg.info.programa);

	if(msgsnd(filaSol, &msg, sizeof(msg), 0) < 0){
		printf("Erro no envio %d\n", errno);
		exit(1);
	}

	sleep(3);

	msg.mtype = 1;
	strcpy(msg.info.programa, "./teste");
	msg.info.seg = 3;

	printf("solicitacao enviada %d %s\n", msg.info.seg, msg.info.programa);

	if(msgsnd(filaSol, &msg, sizeof(msg), 0) < 0){
		printf("Erro no envio %d\n", errno);
		exit(1);
	}
	

	msg.mtype = 1;
	strcpy(msg.info.programa, "./teste");
	msg.info.seg = 10;

	printf("solicitacao enviada %d %s\n", msg.info.seg, msg.info.programa);

	if(msgsnd(filaSol, &msg, sizeof(msg), 0) < 0){
		printf("Erro no envio %d\n", errno);
		exit(1);
	}

	sleep(2);


	msg.mtype = 1;
	strcpy(msg.info.programa, "./teste");
	msg.info.seg = 10;

	printf("solicitacao enviada %d %s\n", msg.info.seg, msg.info.programa);

	if(msgsnd(filaSol, &msg, sizeof(msg), 0) < 0){
		printf("Erro no envio %d\n", errno);
		exit(1);
	}

	sleep(40);

	for(i=0; i<16;i++){
		kill(pids->pids_v[i], SIGUSR1);
	}
	
	kill(pids->pid_esc, SIGUSR1);

	sleep(20);

	for(i=0; i<16; i++){
		if(msgrcv(fila_sh, &estatisticas, sizeof(estatisticas), 0, 0) < 0){
			printf("Erro ao receber estatisticas\n");
			exit(1);
		}

		printf("res[%d]\n", i);
		for(j=0; j<estatisticas.info.total; j++){
			printf("%d    %s    %s    %s     %s\n", estatisticas.info.vetor[j].pid, estatisticas.info.vetor[j].programa, estatisticas.info.vetor[j].tempo_submissao, estatisticas.info.vetor[j].tempo_inicio, estatisticas.info.vetor[j].tempo_fim );	
		}
		printf("\n\n");
	}

	msgctl(fila_sh, IPC_RMID, NULL);
	exit(0);
}