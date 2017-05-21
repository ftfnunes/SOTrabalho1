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


int instacia_gerente_de_execucao(int num_do_gerente){
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
		printf("Fila %d criada\n", (i*10)+i+4);

		if(i < 3 && i/4 == 0){
			if(msgget((i*10)+i+1, IPC_CREAT | 0666) < 0){
				printf("Erro na criacao da fila %d\n", (i*10)+i+1);
				exit(1);
			}
			printf("Fila %d criada\n", (i*10)+i+1);
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


	instancia_filas();

	/*Comunicacao com o node 0*/
	escToNode = msgget(0x334, IPC_CREAT | 0666);

	/*Quando um processo terminar ele envia mensagem pra ca*/
	nodesToEsc = msgget(0x335, IPC_CREAT | 0666);

	shmid = shmget(0x33, 16*sizeof(uint8_t), IPC_CREAT | 0666);
	matriz = shmat(shmid, 0, 0);

	for(i=0; i<16; i++){
		pids[i] = instacia_gerente_de_execucao(i);
	}

	msg.node_dest = 0;
	msg.type = 0;
	time(&(msg.tempo_submissao));
	strcpy(msg.programa, "./teste");

	msgsnd(escToNode, &msg, sizeof(msg), 0);

	exit(0);
}