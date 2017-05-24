# SOTrabalho1

#Compilação

	para compilar todos os programas : make all

	Para compilar o programa executa_postergado: make executa_postergado

	Para compilar o programa shutdown: make shutdown

	Para compilar o programa gerente_de_execucao: make gerente_de_execucao

	Para compilar o programa escalonador: make escalonador


Os executáveis serão criados na pasta bin, que deve ser criada caso nao exista.

##Processo de solicitação de execução

O processo em questão é executado através do executável ./executa_postergado presente na pasta bin, 
e é obrigatório que venha seguido dos parâmetros <seg> que representam o delay de execução e <arq_executavel>
que representa o próprio arquivo a ser executado de maneira postergada. Os parâmetros são validados da 
seguinte maneira:

	1- se o contador de argumentos passados via shell for diferente de 3 (já citados acima);
	2- se o segundo argumento passado, <seg>, é menor que zero o parâmetro é inválido, já que o 
	representa o delay de execução em relação à hora corrente;
	3- verifica se o arquivo executável, terceiro argumento, existe. 

Se alguma dessas validações não for respeitada, uma mensagem informativa é mostrada ao usuário e sai do
programa. Caso contrário, esses valores recebidos via shell são atribuidos a cada campo correspondente em uma estrutura
de dados compartilhada 'mensagem_sol_t'. O processo por fim cria a fila de mensagens com uma chave indicada previamente 
por uma constante numérica representando a fila de mensagens, ou recupera caso já exista, e insere a mensagem na fila que
será acessada pelo escalonador.

##Processo Gerente de Execução

Esse processo representará um nó na estrutura de Torus. Cada nó é representado por um número inteiro, e só poderão se comunicar
com processos vizinhos, condorme definido nessa estruturação.

O processo é criado a partir do escalonador utilizando a função instancia gerente de execução
presente no módulo do escalonador.

No ato da criação de um processo gerente de execução, é necessário passar um inteiro que indicará ao processo que nó este estará
representando dentro do Torus.

Para a comunicação entre os processos dentro do Torus, são utilizadas filas de mensagem, de forma que a cada 2 processos que 
podem se comunicar é criada uma fila de mensagens nova (a criação dessas filas é realizada pelo escalonador). Como nem todos os caminhos 
serão utilizados, só foram criadas filas entre dois processos que se comunicarão de acordo com o algoritmo de roteamento. Cada nó irá 
obter as filas de mensagem que serão efetivamente utilizadas durante a sua execução. As filas tem as suas chaves nomeadas de 0x80 a 
0x8f, de modo que o digito em menos significativo dessa chava especifica o nó ao qual essa fila de mensagem se destina. Dessa forma, são 
criadas 16 filas de mensagem cada uma destinada a um nó. A fila de chave 0x80 será utilizada pelo escalonador para se comunicar com nó 
0.

O algoritmo de roteamento é simplificado, e as rotas utilizadas são mostradas no arquivo Roteamento.jpg enviado no trabalho.

		* Se o nó de destino é o nó atual, para.
		* Se o nó de destino não está na mesma coluna que o nó atual, envia a mensagem para o nó a direita.
		* Caso contrário envia para o nó acima.

Desse modo, fica claro que somente as filas acima e a direita (nos nós de 0 a 2) precisam ser obtidas por cada nó.
	
Cada gerente de execução executa as seguintes funções

		* Rotear mensagens, caso ela não seja direcionada para o nó corrente.
		* Executar o programa contido na mensagem, caso esta seja direcionada para o nó corrente.

A implementação utilizada faz com que, assim que o nó receber uma mensagem destinada a ele, o programa contido na mensagem é executado e 
a próxima mensagem só será lida quando o programa terminar sua execução.

A execução do programa se dá a partir da criação de um processo filho, que será responsável por efetivamente executar o programa. Ao 
termino desse processo filho, o gerente de execução será desbloqueado e as estatisticas referentes a execução desse programa serão 
salvas, enviando as informações referentes a essa execução para o escalonador. 

O processo será terminado quando receber um signal SIGUSER1. Quando esse sinal for recebido, o programa terminará qualquer programa que 
esteja em execução enviando uma mensagem de erro, as estatísticas referentes a todos os programas executados durante o tempo de vida do 
gerente de execução serão enviadas para uma fila de mensagens utilizada pelo processo shutdwown.

##Escalonador:

O escalonador é a base do projeto e executará em background, assim como é requisitado no projeto. Ao ser iniciado, ele registra que o sinal SIGUSR1 fará
com que ele desvie sua execução para a função que finaliza-o, e em seguida gerencia os seguintes recursos:

	* Cria as filas de mensagens entre os gerentes de execução, inclusive a que o escalonador utiliza para se comunicar com o Gerente de execução 0, de 
	acordo com as lógica *explicar a logica das keys*;
	* Cria a fila de mensagem que recebe as solicitações de execução do programa "executa_postergado" (variável: "filaSolicitacoes"), a fila de mensagem 
	que recebe o retorno dos gerentes de execução ao terminarem de executar um programa (variável: "nodesToEsc"), a área de memória compartilhada que 
	irá abrigar os pids dos processos gerentes de execução e do escalonador, com o tipo pids_t, definido em "utils.h" e o semáforo utilizado no projeto.

A partir desse ponto, o escalonador irá instanciar todos os gerentes de execução, armazenando esses pids na área de memória compartilhada, e também 
armazenar o seu próprio pid para que futuramente sejam utilizados no processo de Shutdown com o intuito de serem encerrados.

Todo o processo escalonador se baseia em um loop infinito, fazendo com que seja finalizado somente com a chegada do sinal SIGUSR1. Dentro desse loop, o 
primeiro comando executado é uma recepção de mensagem blocante, ou seja, o escalonador esperará algum processo "executa_postergado" enviar-lhe alguma 
mensagem com o programa a ser executado. 

Assim que recebe tal mensagem, é definido o número de job desse programa, o horário em que a mensagem chegou e em seguida é realizado um fork. Em 
seguida, é definido um trecho de código que será somente executado pelos filhos (há um teste verificando se o pid do fork é zero ou não) que será 
explicado adiante*. 

Após tal trecho, há código que somente a instância mais superior ou a instância pai do escalonador irá executar, armazenando os pids de todos os filhos 
criados e também sua quantidade, executando em loop a função "waitpid" não blocante para remover os possíveis processos zumbis. A partir daí, o loop 
continua voltando à recepção de mensagem blocante dos processos "executa_postergado".

Trecho de código dos processos filhos do escalonador (gerados pelo fork):
	O fork é realizado nesse ponto para que o o processo filho não mude os dados presentes no buffer de mensagem de solicitação de execução, ou seja, 
	preserve a mensagem recebida por seu pai, realizando todo o restante de seu código baseado nesses dados. Isso faz com que o processo pai não precise 
	fazer um controle mais complexo das mensagens recebidas e fique livre para receber mais requisições.

	A primeira instrução dos filhos é registrar que ao receber o sinal SIGUSR2, eles terão seu fluxo desviado para uma rotina de encerramento.
	Os processos filhos do escalonador ficam encarregados de gerenciar o delay até o início da execução do programa, fazendo-o com a instrução "sleep" e 
	com o campo de delay da mensagem de solicitação recebida.

	Nesse ponto, de modo a evitar concorrência no envio de mensagens de para os gerentes de execução e garantir que um programa só será executado quando 
	todos os gerente de execução estejam livres, é realizada a operação "P" do semáforo criado. Assim, o processo que entrar na área protegida pelo 
	semáforo irá cuidar envio das mensagens com o programa a ser executado para o gerente de execução 0 que irá lidar com o reenvio para todos os outros 
	gerentes. 

	Em seguida o filho entrará em loop, e realizará recepções de mensagem blocantes dos gerenciadores de execução, sendo que tais mensagens marcam o fim 
	da execução do programa designado em cada gerente, imprimindo após a recepção os dados do resultado da execução (como número do job e makespan). O 
	loop só se encerrará quando todos os 16 gerentes de execução enviarem a mensagem de solicitação, o que garante que sempre todos eles irão executar o 
	mesmo programa. Após o loop, é executada a operação "V" sobre o  semáforo, liberando a execução de outros programas, e o processo se encerra.

Encerramento:
	Ao receber o sinal SIGUSR1, o escalonador irá remover os recursos utilizados por ele e por outros processos. Todos os gerentes de execução receberão 
	um sinal para finalizarem sua execução. A função de encerramento do escalonador irá remover:

		* Todas as filas de comunicação entre gerentes de execução e a que fica entre o gerente de execução 0 e o escalonador;
		* A fila que o escalonador recebe solicitações de execução;
		- A fila que o escalonador recebe as mensagens de resultado de execução do programa de cada gerente de execução;
		* A área de memória que abriga os pids dos gerentes de execução e do escalonador;
		* O semáforo utilizado;

	Serão realizados 16 wait (blocantes), um parada cada gerente de execução encerrado e em seguida todos os filhos criados pelo escalonador são 
	encerrados, pois recebem o sinal de encerramento, e removidos com wait (blocantes) para cada um dos filhos criados. Foi definido que o escalonador 
	irá executar até no máximo 500 programas, pois esse é o número máximo do vetor que abriga os pids dos filhos do escalonador.

##Shutdown:

	Processo responsável por terminar todos os processos. Constitui-se de quatro partes:

		* inicialização: cria um novo segmento de memória compartilhada, acopla o segmento de memória compartilhada identificado ao segmento de dados do processo que a chamou e atribui o pid dos processos a serem encerrados
		e obtem a fila de mensagens do shutdown vindos do escalonador.
		* envio de sinais para os processos, indicando a eles que devem terminar imediatamente
		* impressão das estatística de cada processo que foi efetivamente executado
		* e por fim,  destrói a fila de mensagens todas as outras operações em curso sobre a fila em questão
		irão falhar e os processos bloqueados em espera de leitura ou escrita serão liberados


