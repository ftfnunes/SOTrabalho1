#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>

#define TIPO_MSG_PIDS 1


struct mensagem {
	long type;
	int node_dest;
	char programa[200];
}


int instacia_gerente_de_execucao(int num_do_gerente){
	int pid;
	char num_gerente_string[3];

	sprintf(num_gerente_string, "%d", num_do_gerente);
	if((pid = fork()) == 0){
		execl("./gerente_de_execucao", "gerente_de_execucao", num_gerente_string, NULL);
	}

	return pid;
}

int instancia_filas(){
	for(i = 0; i<11; i++){
		if(msgget((i*10)+i+4, IPC_CREAT | 0666) < 0){
			printf("Erro na criacao da fila %d\n", (i*10)+i+1);
			exit(1);
		}

		if(i/4 == 0){
			if(msgget((i*10)+i+1, IPC_CREAT | 0666) < 0){
				printf("Erro na criacao da fila %d\n", (i*10)+i+1);
				exit(1);
			}
		}
	}
}


int main(){
	int escToNode, nodesToEsc, shmid;
	int pid;
	int estado;
	uint8_t *matriz;
	struct msg_pids pids_zero;

	pids_zero.type = TIPO_MSG_PIDS;
	pids_zero.pid_direita = 111;
	pids_zero.pid_cima = 112;

	/*Comunicacao com o node 0*/
	escToNode = msgget(0x334, IPC_CREAT | 0666);

	/*Quando um processo terminar ele envia mensagem pra ca*/
	nodesToEsc = msgget(0x335, IPC_CREAT | 0666);

	shmid = shmget(0x33, 16*sizeof(uint8_t), IPC_CREAT | 0666);
	matriz = shmat(shmid, 0, 0);


	pid = instacia_gerente_de_execucao(0);

	msgsnd(escToNode, &pids_zero, sizeof(pids_zero), 0);


	wait(&estado);

	exit(0);
}