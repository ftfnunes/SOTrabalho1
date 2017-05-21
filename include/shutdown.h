#define TAM_PIDS 17
#define PID_ESCALONADOR 0
#define TAM_PROGRAMA 200
#define TAM_TEMPO 20
#define TAM_SHUTDOWN_V 100
#define SHUTDOWN_VECTOR_MTYPE 10

typedef struct {
	uint32_t pids_v[TAM_PIDS];
	uint8_t cont;
} pids_t;

typedef struct {
	uint32_t pid;
	char programa[TAM_PROGRAMA];
	char tempo_inicio[TAM_TEMPO];
	char tempo_fim[TAM_TEMPO];
	char tempo_submissao[TAM_TEMPO];
} shutdown_data_t;

typedef struct {
	long mtype;
	shutdown_data_t vetor[TAM_SHUTDOWN_V];
	uint8_t total;
} shutdown_vector_t;
