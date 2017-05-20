#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_NAME_SZ 256
#define N 20
#define CMD_SHELL "executa_postergado"

void execute(char *, char *);

int main() {

	char line[50], comand[50];
	char * cmd;

	while(1) {

		printf("$ ");
		fgets(line, MAX_NAME_SZ, stdin);
		strcpy(comand, line);

		setbuf(stdin, NULL);
		cmd = strtok(line," \n");

		execute(cmd, comand);
	}

	return 0;
}

void execute(char * cmd, char * comand) {

	int argc = 0;
	char * argv[N];

	while(cmd != NULL) {
		argv[argc] = cmd;												
		cmd = strtok(NULL, " \n");							
		argc++;							
	}

	if(!strcmp(argv[0], "quit")) {
		exit(1);
	}

	/* validação dos parâmetros da solicitação de execução */
    if (argc < 3 || argc > 3 || strcmp(argv[0], CMD_SHELL)) {
    	printf("---------------------------------------------------------\n");
    	printf("\nO primeiro parametro deveria ser 'executa_postergado'\n");
        printf("seguido de um inteiro representando o delay de execucao.\n");
        printf("O parametro seguinte deveria ser o nome do arquivo \n");
        printf("executavel a ser executado de maneira postergada.\n");
        printf("Exemplo:\n");
        printf("  $ executa_postergado 5 hello world\n");
        printf("Ou para sair:\n");
        printf("  $ quit\n\n");
        printf("---------------------------------------------------------\n");
    	
    	return;
    }

}

