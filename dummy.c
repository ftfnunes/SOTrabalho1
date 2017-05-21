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
#include <errno.h>

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

		if(i < 3 && i/4 == 0){
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

		if(i < 3 && i/4 == 0){
			if((id = msgget((i*10)+i+1, 0666)) < 0){
				printf("Erro na remocao da fila %d\n", (i*10)+i+1);
				exit(1);
			}
			msgctl(id, IPC_RMID, NULL);
		}
	}
}


int main(){
	int escToNode, nodesToEsc, shmid;
	int pid, i;
	int estado;
	uint8_t *matriz;
	int pids[16];
	struct mensagem_exe msg;
	struct resultado rst;


	instancia_filas();

	escToNode = msgget(FILA_DO_ESCALONADOR_K, IPC_CREAT | 0666);

	nodesToEsc = msgget(FILA_PARA_ESCALONADOR_K, IPC_CREAT | 0666);

	shmid = shmget(0x33, 16*sizeof(uint8_t), IPC_CREAT | 0666);
	matriz = shmat(shmid, 0, 0);

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

	sleep(10);

	for(i=0; i<16;i++){
		kill(pids[i], SIGKILL);
	}

	remove_filas();

	msgctl(escToNode, IPC_RMID, NULL);
	msgctl(nodesToEsc, IPC_RMID, NULL);

	exit(0);
}