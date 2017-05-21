#include "gerente_de_execucao.h"
#include "utils.h"

int node_num = 0;

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

struct mensagem receber_mensagem(int fila_de_mensagem){
	struct mensagem msg;

	if(msgrcv(fila_de_mensagem, &msg, sizeof(msg), 0, 0) < 0){
		printf("Erro no recebimento de mensagem no node %d\n", node_num);
		exit(1);
	}

	return msg;
}

struct resultado executa_programa(char *programa){
	int pid, estado;
	char *nome_programa;
	struct resultado rst;
	if((nome_programa = strrchr(programa, '/')) == NULL){
		printf("Erro no parse do programa no node %d\n", node_num);
		exit(1);
	}
	
	nome_programa++;


	if((pid = fork()) == 0){
		execl(programa, nome_programa, NULL);
		printf("Erro ao executar o programa\n");
	}

	wait(&estado);



	return rst;
}

void envia_mensagem(struct mensagem msg, int fila_cima, int fila_direita){

	if((node_num%4) == node_num){
		if(msgsnd(fila_cima, &msg, sizeof(msg), 0) < 0){
			printf("Erro no roteamento do node %d\n", node_num);
			exit(1);
		}
	}
	else{
		if(msgsnd(fila_direita, &msg, sizeof(msg), 0) < 0){
			printf("Erro no roteamento do node %d\n", node_num);
			exit(1);
		}	
	}
}

void notifica_escalonador(fila_de_mensagem, rst){
	if(msgsnd(fila_de_mensagem, &rst, sizeof(rst), 0) < 0){
		printf("Erro no roteamento do node %d\n", node_num);
		exit(1);
	}
}


int main(int argc, char** argv){
	int pid = 0;
	int pid_filho = 0;
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
		msg = receber_mensagem(fila_recebimento);

		if(msg.node_dest == node_num){
			matriz_ocupacao[node_num] = 1;
			rst = executa_programa(msg.programa);
			matriz_ocupacao[node_num] = 0;
			notifica_escalonador(fila_para_escalonador, rst);
		}
		else{
			envia_mensagem(msg, fila_cima, fila_direita);
		}
		
	}

	exit(0);
}