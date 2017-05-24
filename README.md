# SOTrabalho1

##Compilação

	para compilar todos os programas : make all

	Para compilar o programa executa_postergado: make executa_postergado

	Para compilar o programa shutdown: make shutdown

	Para compilar o programa gerente_de_execucao: make gerente_de_execucao

	Para compilar o programa escalonador: make escalonador


Os executáveis serão criados na pasta bin, que deve ser criada caso nao exista.

##Processo Gerente de Execução

	Esse processo representará um nó na estrutura de Torus. Cada nó é representado por um número inteiro, e só poderão se comunicar
com processos vizinhos, condorme definido nessa estruturação.
	O processo é criado a partir do escalonador utilizando a função instancia gerente de execução
presente no módulo do escalonador.
	No ato da criação de um processo gerente de execução, é necessário passar um inteiro que indicará ao processo que nó este estará
representando dentro do Torus.
	Para a comunicação entre os processos dentro do Torus, são utilizadas filas de mensagem, de forma que a cada 2 processos que podem se comunicar 
é criada uma fila de mensagens nova (a criação dessas filas é realizada pelo escalonador). Como nem todos os caminhos serão utilizados, só foram criadas filas entre dois processos que se comunicarão de acordo com o algoritmo de roteamento. Cada nó irá obter as filas de mensagem que serão efetivamente utilizadas durante a sua execução. As filas tem as suas chaves nomeadas de 0x80 a 0x8f, de modo que o digito em menos significativo dessa chava especifica o nó ao qual essa fila de mensagem se destina. Dessa forma, são criadas 16 filas de mensagem cada uma destinada a um nó. A fila de chave 0x80 será utilizada pelo escalonador para se comunicar com nó 0.
	O algoritmo de roteamento é simplificado, e as rotas utilizadas são mostradas no arquivo Roteamento.jpg enviado no trabalho.

		* Se o nó de destino é o nó atual, para.
		* Se o nó de destino não está na mesma coluna que o nó atual, envia a mensagem para o nó a direita.
		* Caso contrário envia para o nó acima.

	Desse modo, fica claro que somente as filas acima e a direita (nos nós de 0 a 2) precisam ser obtidas por cada nó.
	
	Cada gerente de execução executa as seguintes funções

		* Rotear mensagens, caso ela não seja direcionada para o nó corrente.
		* Executar o programa contido na mensagem, caso esta seja direcionada para o nó corrente.

	A implementação utilizada faz com que, assim que o nó receber uma mensagem destinada a ele, o programa contido na mensagem é executado e a próxima 
mensagem só será lida quando o programa terminar sua execução.
	A execução do programa se dá a partir da criação de um processo filho, que será responsável por efetivamente executar o programa. Ao termino desse
processo filho, o gerente de execução será desbloqueado e as estatisticas referentes a execução desse programa serão salvas, enviando as informações referentes a essa execução para o escalonador. 
	O processo será terminado quando receber um signal SIGUSER1. Quando esse sinal for recebido, o programa terminará qualquer programa que esteja em
execução enviando uma mensagem de erro, as estatísticas referentes a todos os programas executados durante o tempo de vida do gerente de execução serão enviadas para uma fila de mensagens utilizada pelo processo shutdwown.
