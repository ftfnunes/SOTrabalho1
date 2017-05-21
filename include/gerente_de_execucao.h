#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>


#define FILA_DO_ESCALONADOR_K 0X334
#define FILA_PARA_ESCALONADOR_K 0X335
#define TRUE 1