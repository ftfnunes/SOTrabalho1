#include "executa_postergado.h"
#include "utils.h"

int main(int argc, char* argv[]) {

	mensagem_sol_t msg;
	int msqid;
	char * path = "executa_postergado";
 
	/* validação dos parâmetros da solicitação de execução    */
    if (argc != 3) {

        printf("\nO primeiro parametro deveria ser fornecido como\n");
        printf("inteiro representando o delay de execucao.\n");
        printf("O parametro seguinte deve ser o nome do arquivo \n");
        printf("executavel a ser executado de maneira postergada.\n");
        printf("Exemplo:\n");
        printf("\t> executa_postergado 5 hello world\n\n");

        exit(1);

    } else if(atoi(argv[1]) < 0 || atoi(argv[2]) < 0) { 	
    	printf("\nParametro invalido.\n");
    	exit(1);

    } 

    msg.mtype = 1;
	msg.info.seg = atoi(argv[1]);
	strcpy(msg.info.programa, argv[2]);

	if((msqid = msgget(FILA_SOLICITACAO_K, 0666)) < 0) {
		printf("Erro na criação da fila.\n");
	}

	if(msgsnd(msqid ,&msg, sizeof(msg), 0) < 0){
		printf("Erro no envio de solicitacao\n");
	}

  	return 0;
}


