#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>

#define FILA_DO_ESCALONADOR_K 0X334
#define FILA_PARA_ESCALONADOR_K 0X335
#define TRUE 1

struct mensagem {
	long type;
	int node_dest;
	char programa[200];
}

struct resultado{
	long type;
	char tempo_inicio[20];
	char tempo_fim[20];
	int turnaround;
}

/*As chaves das filas de msg sao criadas a partir do algoritmo (key = 10*no_origem + no_destino)*/
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
	int fila_para_escalonador = -1;
	int fila_direita = -1;
	int fila_cima = -1;
	int fila_recebimento = -1;
	int estado;
	struct resultado rst;
	struct mensagem msg;
	uint8_t *matriz_ocupacao;

	if(argc != 2){
		printf("Gerente de execucao iniciado incorretamente\n");
		exit(1);
	}

	node_num = atoi(argv[1]);

	if((shmid = shmget(0x33, 16*sizeof(uint8_t), 0666)) < 0){
		printf("Erro ao se obter segmento de memoria compartilhada\n");
		exit(1);
	}
	
	if((fila_para_escalonador = msgget(FILA_PARA_ESCALONADOR_K, 0666)) < 0){
		printf("Erro ao se obter fila de menssagens\n");
		exit(1);
	}

	if((matriz_ocupacao = (uint8_t *)shmat(shmid, 0, 0)) == NULL){
		printf("Erro no attach\n");
		exit(1);
	}


	/*As filas devem ser criadas antes da criacao dos processos (IPC_CREAT)*/
	if((node_num/4) == 0){
	/*node zero recebe, do escalonador */
		if(node_num == 0){
			if((fila_recebimento = msgget(FILA_DO_ESCALONADOR_K, 0666)) < 0){
				printf("Erro ao se obter a fila de mensagens\n");
				exit(1);
			}
		}			
		else{
			if((fila_recebimento = msgget((node_num-1)*10 + node_num, 0666)) < 0){
				printf("Erro ao se obter a fila de mensagens\n");
				exit(1);
			}
		}
	}			
	else{
		if((fila_recebimento = msgget((node_num-4)*10 + node_num, 0666)) < 0){
			printf("Erro ao se obter a fila de mensagens\n");
			exit(1);
		}
	}

	if(node_num/4 < 3){
		if((fila_cima = msgget((node_num*10)+node_num+4, 0666)) < 0){
			printf("Erro ao se obter a fila de mensagens\n");
			exit(1);
		}
	}

	if(node_num != 3 && node_num/4 == 0){
		if((fila_direita = msgget((node_num*10)+node_num+1, 0666)) < 0){
			printf("Erro ao se obter a fila de mensagens\n");
			exit(1);
		}
	}

	while(TRUE){
		receber_mensagem(fila_recebimento, &msg);

		if(msg.node_dest == node_num){
			matriz_ocupacao[node_num] = 1;
			rst = executa_programa(node.programa);
			notifica_escalonador(fila_para_escalonador, rst);
		}
		else{
			envia_mensagem(msg.node_dest, fila_cima, fila_direita);
		}
		
	}

	exit(0);
}