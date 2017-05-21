#include "gerente_de_execucao.h"
#include "utils.h"

extern int errno;

int node_num = 0;
int pid_prog = 0;
int pid_gerente = 0;
uint8_t *matriz_ocupacao;
mensagem_exec_t msg;
shutdown_vector_t estatisticas;

/*As chaves das filas de msg sao criadas a partir do algoritmo (key = 10*no_origem + no_destino)*/
int instancia_gerente_de_execucao(int num_do_gerente){
	int pid;
	char num_gerente_string[3];

	sprintf(num_gerente_string, "%d", num_do_gerente);
	if((pid = fork()) == 0){
		execl("./gerente_de_execucao", "gerente_de_execucao", num_gerente_string);
	}

	return pid;
}

mensagem_exec_t receber_mensagem(int fila_de_mensagem){
	mensagem_exec_t msg;

	if(msgrcv(fila_de_mensagem, &msg, sizeof(msg), 0, 0) < 0){
		printf("Erro no recebimento de mensagem no node %d\n", node_num);
		exit(1);
	}

	/*printf("%d recebeu a msg! node dest: %d\n", node_num, msg.info.node_dest);
*/
	return msg;
}

resultado_t executa_programa(char *programa){
	int estado;
	time_t inicio, fim;
	char *nome_programa;
	resultado_t rst;

	if((nome_programa = strrchr(programa, '/')) == NULL){
		printf("Erro no parse do programa no node %d\n", node_num);
		exit(1);
	}
	
	nome_programa++;

	time(&inicio);

	if((pid_prog = fork()) == 0){
		execl(programa, nome_programa, NULL);
		printf("Erro ao executar o programa no node %d\n", node_num);
		exit(1);
	}

	wait(&estado);

	time(&fim);

	rst.info.turnaround = (long)(fim-inicio);
	rst.info.node = node_num;
	rst.mtype = 1;

	strcpy(rst.info.inicio, ctime(&inicio));
	strcpy(rst.info.fim, ctime(&fim));
	rst.info.inicio[strlen(rst.info.inicio)-1] = '\0';
	rst.info.fim[strlen(rst.info.fim)-1] = '\0';

	strcpy(estatisticas.info.vetor[estatisticas.info.total].tempo_inicio, rst.info.inicio);
	strcpy(estatisticas.info.vetor[estatisticas.info.total].tempo_fim, rst.info.fim);
	strcpy(estatisticas.info.vetor[estatisticas.info.total].tempo_submissao, ctime(&(msg.info.tempo_submissao)));
	estatisticas.info.vetor[estatisticas.info.total].tempo_submissao[strlen(estatisticas.info.vetor[estatisticas.info.total].tempo_submissao) - 1] = '\0';
	strcpy(estatisticas.info.vetor[estatisticas.info.total].programa, programa);
	estatisticas.info.vetor[estatisticas.info.total].pid = pid_prog;

	estatisticas.info.total++;

	return rst;
}

void envia_mensagem(mensagem_exec_t msg, int fila_cima, int fila_direita){

	if(node_num > 3 || (msg.info.node_dest%4) == node_num){
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

void notifica_escalonador(int fila_de_mensagem, resultado_t rst){
	if(msgsnd(fila_de_mensagem, &rst, sizeof(rst), 0) < 0){
		printf("Erro no roteamento do node %d\n", node_num);
		exit(1);
	}
}

void trata_shutdown(){
	int fila_shutdown = -1;
	if(matriz_ocupacao[node_num] == 1){
		kill(pid_prog, SIGKILL);
		printf("O programa %s nao completou sua execucao no node %d\n", msg.info.programa, node_num);
	}
	
	if((fila_shutdown = msgget(0x120700, 0666)) < 0){
		printf("Erro na obtencao da fila de shutdown no node %d\n", node_num);
		exit(1);
	}

	if(msgsnd(fila_shutdown, &estatisticas, sizeof(estatisticas), 0) < 0){
		printf("Erro ao enviar estatisticas do processo %d com erro %d\n", node_num, errno);
		exit(1);
	}

	exit(0);
}


int main(int argc, char** argv){
	int shmid = 0;
	int fila_para_escalonador = -1;
	int fila_direita = -1;
	int fila_cima = -1;
	int fila_recebimento = -1;
	resultado_t rst;

	if(argc != 2){
		printf("Gerente de execucao iniciado incorretamente\n");
		exit(1);
	}

	signal(SIGUSR1, trata_shutdown);

	node_num = atoi(argv[1]);
	pid_gerente = getpid();
	estatisticas.info.total = 0;
	estatisticas.mtype = 1;


	if((shmid = shmget(0x33, 16*sizeof(uint8_t), 0666)) < 0){
		printf("Erro ao se obter segmento de memoria compartilhada no node %d\n", node_num);
		exit(1);
	}
	
	if((fila_para_escalonador = msgget(FILA_PARA_ESCALONADOR_K, 0666)) < 0){
		printf("Erro ao se obter fila de menssagens no node %d\n", node_num);
		exit(1);
	}

	if((matriz_ocupacao = (uint8_t *)shmat(shmid, 0, 0)) == NULL){
		printf("Erro no attach no node %d\n", node_num);
		exit(1);
	}


	/*As filas devem ser criadas antes da criacao dos processos (IPC_CREAT)*/
	if(node_num < 4){
	/*node zero recebe, do escalonador */
		if(node_num == 0){
			if((fila_recebimento = msgget(FILA_DO_ESCALONADOR_K, 0666)) < 0){
				printf("Erro ao se obter a fila de mensagens no node %d\n", node_num);
				exit(1);
			}
		}			
		else{
			if((fila_recebimento = msgget((node_num-1)*10 + node_num, 0666)) < 0){
				printf("Erro ao se obter a fila de mensagens no node %d\n", node_num);
				exit(1);
			}
		}
	}			
	else{
		if((fila_recebimento = msgget((node_num-4)*10 + node_num, 0666)) < 0){
			printf("Erro ao se obter a fila de mensagens no node %d\n", node_num);
			exit(1);
		}
	}

	if(node_num < 12){
		if((fila_cima = msgget((node_num*10)+node_num+4, 0666)) < 0){
			printf("Erro ao se obter a fila de mensagens no node %d\n", node_num);
			exit(1);
		}
	}

	if(node_num < 3){
		if((fila_direita = msgget((node_num*10)+node_num+1, 0666)) < 0){
			printf("Erro ao se obter a fila de mensagens no node %d\n", node_num);
			exit(1);
		}
	}

	/*printf("Todos os recursos setados! (%d)\n", node_num);
*/
	/*A partir que o processo recebe a requisao de executar ele para de rotear*/
	while(TRUE){
		msg = receber_mensagem(fila_recebimento);
		if(msg.info.node_dest == node_num){
			if(matriz_ocupacao[node_num] == 0){
				matriz_ocupacao[node_num] = 1;
				rst = executa_programa(msg.info.programa);

				matriz_ocupacao[node_num] = 0;
				notifica_escalonador(fila_para_escalonador, rst);
			}			
		}
		else{
			envia_mensagem(msg, fila_cima, fila_direita);
		}
		
	}

	exit(0);
}