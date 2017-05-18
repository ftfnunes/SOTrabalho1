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

struct msg_pids{
	long type;
	int pid_direita;
	int pid_cima;
};

int pid_vizinhos[4];

int instacia_gerente_de_execucao(int num_do_gerente){
	int pid;
	char num_gerente_string[3];

	sprintf(num_gerente_string, "%d", num_do_gerente);
	if((pid = fork()) == 0){
		execl("./gerente_de_execucao", "gerente_de_execucao", num_gerente_string);
	}

	return pid;
}

int main(int argc, char** argv){
	int pid = 0;
	int pid_filho = 0;
	int node_num = 0;
	int shmid = 0;
	int mensagens_do_escalonador = 0;
	int mensagens_para_escalonador = 0;
	int estado;
	uint8_t *matriz_ocupacao;
	struct msg_pids pids_vizinhos;

	if(argc != 2){
		printf("Gerente de execucao iniciado incorretamente\n");
		exit(1);
	}

	node_num = atoi(argv[1]);

	printf("%d\n", node_num);

	shmid = shmget(0x33, 16*sizeof(uint8_t), 0666);

	if(node_num == 0){
		mensagens_do_escalonador = msgget(0x34, 0666);
	}
	
	mensagens_para_escalonador = msgget(0x35, 0666);

	matriz_ocupacao = (uint8_t *)shmat(shmid, 0, 0);

	/*node zero recebe, do escalonador */
	if(node_num == 0){
		if(msgrcv(mensagens_do_escalonador, &pids_vizinhos, sizeof(pids_vizinhos), TIPO_MSG_PIDS, 0) < 0){
			printf("Erro ao receber mensagem\n");
			exit(1);
		}
	}

	printf("%d %d\n", pids_vizinhos.pid_direita, pids_vizinhos.pid_cima);

	exit(0);
}