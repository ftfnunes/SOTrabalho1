#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>


int instacia_gerente_de_execucao(int num_do_gerente){
	int pid;
	char num_gerente_string[3];

	sprintf(num_gerente_string, "%d", num_do_gerente);
	if((pid = fork()) == 0){
		execl("./gerente_de_execucao", "gerente_de_execucao", num_gerente_string, NULL);
	}

	return pid;
}


int main(){
	int escToNode, shmid;
	int pid;
	int estado;
	uint8_t *matriz;


	/*Comunicacao com o node 0*/
	escToNode = msgget(0x34, IPC_CREAT | 0666);

	/*Quando um processo terminar ele envia mensagem pra ca*/
	nodesToEsc = msgget(0x35, IPC_CREAT | 0666);

	shmid = shmget(0x33, 16*sizeof(uint8_t), IPC_CREAT | 0666);
	matriz = shmat(shmid, 0, 0);


	pid = instacia_gerente_de_execucao(0);

	sleep(5);

	printf("%d\n", matriz[0]);

	wait(&estado);

	exit(0);
}