#include <stdio.h>

int main(int argc, char* args[]) {


	// validação dos parâmetros da solicitação de execução    
    if (argc <= 2 || argc > 3) {
        printf("\nO primeiro parametro deveria ser fornecido como\n");
        printf("inteiro representando o delay de execucao.\n");
        printf("O parametro seguinte deve ser o nome do arquivo \n");
        printf("executavel a ser executado de maneira postergada.\n");
        printf("Exemplo:\n");
        printf("\t> executa_postergado 5 hello world\n\n");
        return 0;
    }

	printf("passou\n");    

	return 0;
}