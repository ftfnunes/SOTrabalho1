#include "gerente_de_execucao.h"
#include "utils.h"

extern int errno;

int node_num = 0; /*node_num: numero de identificacao deste gerente de execucao dentro do torus, para fins de roteamento*/
int pid_prog = 0; /*pid_prog: pid do processo (criado por este gerente de execucao) que executa o programa delegado pelo escalonador*/
int pid_gerente = 0; /*pid_gerente: pid deste processo gerente de execucao*/
int old_total = -1; /*(explicacao do old_total na funcao executa_programa)*/
mensagem_exec_t msg; /*msg: mensagem de execucao de um programa, enviada primeiramente pelo escalonador, e roteada pelos nodes ate chegar no destino*/
shutdown_vector_t estatisticas; /*estatisticas: mensagem com as estatisticas relativas as execucoes de todos os programas de um gerente, que serao enviadas para o modulo Shutdown*/

/*Este gerente recebe uma mensagem de outro gerente do Torus (ou do escalonador, caso esse node seja o zero)*/
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

/*cria um processo a partir de um fork, que executara o programa o qual o escalonador passou para os gerentes*/
resultado_t executa_programa(char *programa){
	int estado;
	time_t inicio, fim;
	char *nome_programa;
	resultado_t rst;

	/*old_total serve para sabermos se um programa esta sendo executado ou preparado pelo gerente*/
	/*caso old_total seja igual a estatisticas.info.total, ha um programa em execucao/preparacao (no final desta funcao, estatisticas.info.total eh incrementado)*/
	old_total = estatisticas.info.total;

	/*nessa etapa, estamos pegando o nome do programa do path (variavel "programa") que nos foi passado*/
	/*ex: programa = ./teste  -->  nome_programa = teste*/
	if((nome_programa = strrchr(programa, '/')) == NULL){
		printf("Erro no parse do programa no node %d\n", node_num);
		exit(1);
	}
	
	nome_programa++;

	/*pega o tempo de inicio de execucao*/
	time(&inicio);

	/*dah o fork e executa o programa desejado*/
	if((pid_prog = fork()) == 0){
		execl(programa, nome_programa, NULL);
		printf("Erro ao executar o programa no node %d\n", node_num);
		exit(1);
	}

	/*pega o estado de termino do programa (mata ele com o wait) e pega o tempo de termino de execucao*/
	wait(&estado);
	time(&fim);

	/*as etapas abaixo sao de preenchimento das structs de informacoes de feedback que serao passadas futuramente para o escalonador e para o modulo de shutdown*/
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
	estatisticas.info.vetor[estatisticas.info.total].tempo_submissao[strlen(estatisticas.info.vetor[estatisticas.info.total].tempo_submissao)-1] = '\0';
	strcpy(estatisticas.info.vetor[estatisticas.info.total].programa, programa);
	estatisticas.info.vetor[estatisticas.info.total].pid = pid_prog;

	/*incrementa o indice que representa o numero de programas jah executados por este gerente de execucao*/
	/*vale lembrar que, a partir daqui, old_total != estatisticas.info.total, indicando assim que esse gerente nao esta mais executando/preparando nenhum programa*/
	estatisticas.info.total++;

	return rst;
}

void envia_mensagem(mensagem_exec_t msg, int fila_cima, int fila_direita){

	/*se o node atual for mais que 3, so existem caminhos de envio para nos de cima, entao envia pra cima*/
	/*se o node atual for menor que 3, mas o node de destino estiver acima do atual (node_dest%4 == node_num), entao envia a mensagem pra cima*/
	if(node_num > 3 || (msg.info.node_dest%4) == node_num){
		if(msgsnd(fila_cima, &msg, sizeof(msg), 0) < 0){
			printf("Erro no roteamento do node %d com erro %d para node %d\n", node_num, errno, msg.info.node_dest);
			exit(1);
		}
	}
	else{ /*nesse caso, temos certeza que 0 <= node_num <= 2, e que o node destino nao esta acima do node atual, entao, temos que enviar pro node da esquerda*/
		if(msgsnd(fila_direita, &msg, sizeof(msg), 0) < 0){
			printf("Erro no roteamento do node %d com erro %d para node %d\n", node_num, errno, msg.info.node_dest);
			exit(1);
		}	
	}

	/*printf("%d enviou a msg! node dest: %d\n", node_num, msg.info.node_dest);
*/
}

/*Depois que o processo que foi criado por este gerente termina sua execucao, a funcao abaixo envia as informacoes de feedback da execucao para o escalonador*/
void notifica_escalonador(int fila_de_mensagem, resultado_t rst){
	if(msgsnd(fila_de_mensagem, &rst, sizeof(rst), 0) < 0){
		printf("Erro no roteamento do node %d\n", node_num);
		exit(1);
	}
}

/*funcao que tratara um sinal de shutdown*/
void trata_shutdown(){
	int fila_shutdown = -1;
	
	/*como explicado na funcao executa_programa, se old_total == estatisticas.info.total, entao havia um programa sendo preparado/executado. (neste caso, mandaremos SIGKILL para o proc. desse programa)*/
	/*(como estatisticas.info.total nao chegou a ser incrementado, entao as estatisticas dele nao serao enviadas para o modulo de Shutdown)*/
	if(old_total == estatisticas.info.total){
		kill(pid_prog, SIGKILL);
		printf("O programa %s nao foi finalizado no node %d\n", msg.info.programa, node_num);
	}
	
	/*As etapas abaixo sao as de obtencao da fila e entao envio das estatisticas para o modulo de shutdown*/

	if((fila_shutdown = msgget(FILA_SHUTDOWN_K, 0666)) < 0){
		printf("Erro na obtencao da fila de shutdown no node %d\n", node_num);
		exit(1);
	}

	if(msgsnd(fila_shutdown, &estatisticas, sizeof(estatisticas), 0) < 0){
		printf("Erro ao enviar estatisticas do processo %d com erro %d\n", node_num, errno);
		exit(1);
	}

	/*termina a execucao desse gerente de execucao*/
	exit(0);
}


int main(int argc, char** argv){
	int fila_para_escalonador = -1;
	int fila_direita = -1;
	int fila_cima = -1;
	int fila_recebimento = -1;
	resultado_t rst; /*rst (resultado): mensagem de feedback da execucao de um programa para o escalonador*/

	/*esperamos receber, como argumento, o numero desse gerente de execucao, para sua identificacao no Torus*/
	if(argc != 2){
		printf("Gerente de execucao iniciado incorretamente\n");
		exit(1);
	}

	/*indica que, ao receber o sinal de shutdown (usado por nos como o SIGUSR1), trata ele com a rotina trata_shutdown, para terminar tudo corretamente*/
	signal(SIGUSR1, trata_shutdown);

	node_num = atoi(argv[1]);
	pid_gerente = getpid();

	estatisticas.info.total = 0;
	estatisticas.mtype = 1;

	/*As etapas abaixo se referem a obtencao das filas necessarias*/

	/*essa fila sera usada para o gerente mandar a notificacao de feedback para o escalonador*/
	if((fila_para_escalonador = msgget(FILA_PARA_ESCALONADOR_K, 0666)) < 0){
		printf("Erro ao se obter fila de menssagens no node %d\n", node_num);
		exit(1);
	}

	/*fila usada para os nodes do Torus receberem mensagens uns dos outros. (O node zero vai receber msg do escalonador)*/
	if((fila_recebimento = msgget(FILA_0_K+node_num, 0666)) < 0){
		printf("Erro ao se obter a fila de recebimento no node %d\n", node_num);
		exit(1);
	}
	/*printf("No %d obteve fila de recebimento 0x%x\n", node_num, FILA_0_K+node_num);*/

	/*os nodes do 0 ao 11 (nodes da primeira linha a terceira linha) vao ter um caminho de comunicacao (fila de msg) para cima*/
	if(node_num < 12){
		if((fila_cima = msgget(FILA_0_K+node_num+4, 0666)) < 0){
			printf("Erro ao se obter a fila de cima no node %d\n", node_num);
			exit(1);
		}
		/*printf("No %d obteve fila de cima 0x%x\n", node_num, FILA_0_K+node_num+4);*/
	}

	/*os tres primeiros nodes da primeira linha (0, 1 e 2) vao ter um caminho de comunicacao (fila de msg) para a direita*/
	if(node_num < 3){
		if((fila_direita = msgget(FILA_0_K+node_num+1, 0666)) < 0){
			printf("Erro ao se obter a fila da direita no node %d\n", node_num);
			exit(1);
		}
		/*printf("No %d obteve fila da direita 0x%x\n", node_num, FILA_0_K+node_num+1);*/
	}

	/*printf("Todos os recursos setados! (%d)\n", node_num);*/

	/*Com todos os recursos obtidos, o gerente agora executara o seu loop principal, com os seguintes passos: */
	/*Recebe msg de execucao e ve se a msg eh pra ele; caso seja, ele executara esse programa; caso nao seja, envia a msg pra outro node*/
	while(TRUE){
		msg = receber_mensagem(fila_recebimento);
		if(msg.info.node_dest == node_num){
			rst = executa_programa(msg.info.programa);
			/*depois da execucao do programa, envia o respectivo feedback (rst) para o escalonador*/
			notifica_escalonador(fila_para_escalonador, rst);
		}
		else{
			envia_mensagem(msg, fila_cima, fila_direita);
		}
		
	}

	exit(0);
}