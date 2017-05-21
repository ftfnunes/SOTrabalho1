#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

struct mensagemSol{
	long type;
	int seg;
	char programa[200];
};
typedef struct mensagemSol MsgSol;

struct mensagemExe{
	long type;
	int node_dest;
	char programa[200];
};
typedef struct mensagemExe MsgExe;

/*
	Mecanismo para escalonar os processos a serem executados.

	Key para obtenção da fila de mensagens: A_DEFINIR .

	É necessário definir a estrutura de dados que irá estar na mensagem recebida pelo gerenciador de solicitação de execução:

	struct solicitacao{
		long pid;
		//Estrutura da mensagem
	}

	Também é necessário definir a estrutura a ser enviada para os gerenciadores de execução
*/


int main(){
	int filaSolicitacoes, filaExecucao, testeFila, estado, pid, *mtzGerentesExec, shmid;
	MsgSol msgSolicitacao;
	MsgExe msgExecucao[30];


	// Criação da fila de mensagens que recebe as solicitações de execução vindas dos processos de solicitação de execução.
	if(filaSolicitacoes = msgget(0x1, IPC_CREAT | 0666) < 0){
		printf("Erro na criação da fila.\n");
		exit(1);
	}

	// Criação da fila de mensagens para envio das mensagens com os programas a serem executados para os gerentes de execução.
	if(filaExecucao = msgget(0x, IPC_CREAT | 0666) < 0){
		printf("Erro na criação da fila.\n");
		exit(1); 
	}

	// Criação de um vetor de inteiros em memória compartilhada para verificar se os gerentes de execução estão livres.
	if(shmid = shmget(0x33, 16*sizeof(uint8_t), IPC_CREAT | 0666)){
		printf("Erro na criação de memória compartilhada.\n");
		exit(1);
	}

	// Atribuição à variável "mtzGerentesExec" a área de memória compartilhada que abriga o vetor com o status dos gerentes de execução, se estão livre ou nao
	if(mtzGerentesExec = shmat(shmid, 0, 0)){
		printf("Erro na atribuição de memória compartilhada.\n");
		exit(1);
	}

	while(1){
		msgrcv(filaSolicitacoes, &msgSolicitacao, sizeof(msgSolicitacao), 0, 0);

		pid = fork();

		if(pid == 0){
			int i, sai = 0, conta_livres;
			MsgExe msgs[16];
			sleep(msgSolicitacao.seg);

			//Loop verifica se todos os gerenciadores de processo estão livres.
			while(!sai){
				conta_livres = 0;
				for(i = 0; i < 16; ++i){
					if(mtzGerentesExec[i] == 0 )
						++conta_livres;
				}
				if(conta_livres == 16)
					sai = 1;
			}

			for(i = 0; i < 16; ++i) {
				msgs[i].type = getpid();
				strcpy(msgs[i].programa, msgSolicitacao.programa);
				msgs[i].node_dest = i;
				//Falta criar o comando para enviar a mensagem.
			}

			/*Não estamos verificando se a fila de mensagens está cheia. */
			msgsnd(filaExecucao, &msgSolicitacao, sizeof(msgSolicitacao), IPC_NOWAIT);

			exit(0);
		}

		/* Registrando o wait com o PID do filho antes de continuar o loop. */
		waitpid(pid, &estado, WNOWAIT);
	}

	return 0;
}

int exec_proc();