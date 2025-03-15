#pragma once

/* Este trabalho visa implementar uma simulação do algoritmo de escalonamento de tarefas Round Robin, 
   junto com um gerenciador de memória utilizando paginação lógica e física. 
   
   Ao final da execução, é gerado um relatório detalhado com métricas de desempenho.
*/

#include <stdio.h>

// Tamanho máximo de uma linha lida.
#define MAX_LINE 30

// Tamanho máximo para o nome de cada arquivo (tarefa).
#define MAX_NAME 20

// Quantidade máxima de tasks que podem ser informadas na linha de comando.
#define MAX_TASKS 4

// Quantidade máxima de instruções para cada tarefa.
#define MAX_INSTRUCTIONS 50

// Memória física total (64 KB).
#define MAX_TOTAL_MEMORY 65536  

// Memória lógica da maior tarefa (4 KB).
#define MAX_MEMORY_TASK 4096

// Tamanho de cada página (512 bytes).
#define PAGE_SIZE 512  

// Quantidade máxima de páginas.
#define TOTAL_PAGES 128  

// Tamanho da memória física reservada para o programa (20 KB).
#define MAX_PROGRAM_MEMORY 20480    

// Intervalo de endereços físicos para o programa.
#define BEGIN_PROGRAM_ADDRESS 0
#define END_PROGRAM_ADDRESS 20479

// Número de páginas físicas para o programa.
#define MAX_PROGRAM_PAGES 40

// Tamanho da memória física reservada para as tarefas (44 KB).
#define TASK_MEMORY 45056         

// Intervalo de endereços físicos para as tarefas.
#define BEGIN_TASKS_ADDRESS 20480
#define END_TASKS_ADDRESS 65535

// Número de páginas físicas.
#define MAX_TASK_PAGES 88         

// Unidade de tempo equivalente à 1 instrução da tarefa.
#define UT 1

// Tamanho de uma fatia de tempo.
#define QUANTUM_UT 2        

// Endereço lógico inicial.
#define INITIAL_ADDRESS 0

// Tempo de espera a tarefa para poder voltar ao estado apto.
#define IO_WAIT_TIME 5      

// Quantidade de índices
#define INDEX_LOGICAL_PHYSICAL 2

// Região.
#define LOCALE "pt_BR.utf8"

// Formato que indica o tamanho da memória inicial requerida pela tarefa.
#define SIZE "#T="

// Tipos de instruções.
#define NEW "new"
#define READ "read disk"

// Status de uma tarefa.
#define TERMINATED 0
#define READY 1
#define RUNNING 2
#define SUSPENDED 3
#define COMPLETED 4
#define INVALID -1
#define INVALID_TASK -2
#define INVALID_ACCESS -3
#define INVALID_ALLOCATION -4

// Erros de arquivo.
#define NOT_OPEN_FILE -1
#define EMPTY_FILE -2
#define INVALID_FILE -3

// Tipos de instrução.
#define MEMORY_ALLOCATION 1
#define MEMORY_ACCESS 2 
#define DISK_ACCESS 3

// Índices correspondentes.
#define LOGICAL 0
#define PHYSICAL 1

typedef struct {
   float waitingTime;     // Tempo total de entrada/saída gasto pelo programa.
   float turnaroundTime;  // tempo de CPU gasto pelo programa.
   float IOTime;          // Tempo de entrada e saída gasto em todo o programa.
} TimeProgram;

typedef struct {
   int lastPhysicalAddress;   // Último endereço físico alocado.
} MemoryProgram;

typedef struct {
   int cpuTimeTask;       // Tempo total de processador que a tarefa necessita para execução de suas instruções.
   int IOtimeTask;        // Tempo total necessário para a tarefa realizar todas as operações de entrada e saída de dados, ou seja, instruções do tipo 3.
   int remainingTimeIO;   // Tempo restante para completar a E/S atual (se houver).
   int timeEnteredQueue;  // Instante em que a tarefa entrou na fila de prontas.
   int waitTimeTask;      // Tempo de espera da terafa.
} TimeTask;

typedef struct {
   int page[INDEX_LOGICAL_PHYSICAL];     // Mapeamento página lógica e física.
   int offset[INDEX_LOGICAL_PHYSICAL];   // Mapeamento deslocamento lógico e físico.
   // Endereço lógico inicial e final.
   int beginLogicalAddress;              
   int endLogicalAddress;      
   // Endereço físico inicial e final.
   int beginPhysicalAddress;             
   int endPhysicalAddress;     
} AddressRange;

typedef struct {
   int allocatedDate;                            // Quantidade de memória de dados alocada. 
   int allocatedCode;                            // Quantidade de memória de código alocada.
   int allocatedMemory;                          // Quantidade de memória alocada.
   int totalPages;                               // Quantidade total de páginas utilizadas na tarefa.
   int usedCodePages;                            // Quantidade de páginas usadas para código.
   int usedDataPages;                            // Quantidade de páginas usadas para dados.                           
   AddressRange pagesTable[MAX_TASK_PAGES];  // Mapeamento de páginas de lógicas e físicas com seus respectivos endereços.
} MemoryTask;

typedef struct {
   char nameInstructions[MAX_LINE];  // Armazenaa as instruções da tarefa.
   AddressRange adressInstruction;   // Endereços de memória de cada instrução.

} Instructions;

typedef struct {
   char name[MAX_NAME];                              // Nome da tarefa.
   int status;                                       // Status da tarefa.
   int ut;                                           // Unidade de tempo gasta pela tarefa.
   FILE *file;                                       // Aponta para o arquivo na memória.
   char arrays[MAX_INSTRUCTIONS][MAX_NAME];          // Armazena os vetores criados.
   int arraysSize[MAX_INSTRUCTIONS];                 // Armazena o tamanho de cada vetor criado.
   int arrayCount;                                   // Quantidade de vetores criados.
   int instructionCount;                             // Resultado da contagem de instruções.
   Instructions instructionsTask[MAX_INSTRUCTIONS];  // Vetor que conterá as instruções, a página e os endereços respectivos.
   TimeTask timeTask;                                // Valores de tempo da tarefa.
   MemoryTask memoryTask;                            // Endereços de cada página da tarefa.
} Tasks; 

typedef struct Node {
   Tasks task;
   struct Node *next;
} Node;

typedef struct {
   Node *front, *back;
   int size;
} Queue;

typedef struct {
   Queue *readyQueue;      // Fila de tarefas prontas para receber o processador.
   Queue *suspendedQueue;  // Fila de tarefas suspensas.
} Scheduler;

// Mensagens de erro.
#define ERR_NO_PARAMETER "\nParâmetro inválido.\nUso: %s <arquivo(s)>\n\n"
#define ERR_MAX_PARAMETER "\nA quantidade de parâmetros execedeu o limite.\nLimite = 4 arquivos.\n\n"
#define ERR_TASK_INVALID "\nA tarefa %s não será executada, pois tem instruções diferentes do tipo 1, 2 e 3.\n"
#define ERR_ACCESS_MEMORY "\nA tarefa %s tentou realizar um acesso inválido à memória v[%d].\n"
#define ERR_PARSE_LINE "\nErro ao analisar linha de código: %s.\n"
#define ERR_ACCESS_ARRAY "\nA tarefa %s tentou acessar um vetor inexistente.\n"
#define ERR_MAX_ARRAYS "\nA tarefa excedeu o limite de vetores a serem criados (%d).\n"
#define ERR_ALOCATE_MEMORY "\nA tarefa %s excedeu o limite de memória reservada para as tarefas (%d).\n"
#define ERR_INVLID_FILE "\nArquivo %s inválido.\n"
#define ERR_FILE_EMPTY "\nO arquivo %s está vazio.\n"
#define ERR_OPEN_FILE "\nErro ao abrir arquivo %s.\n"

/* Função principal que inicializa o sistema. 

   Parâmetros:
   - amount_tasks: número de tarefas a serem processadas.
   - argv: vetor de strings contendo os nomes dos arquivos que representam as tarefas.
 
   Retorno:
   - Retorna 0 em caso de sucesso. Caso contrário, retorna um código de erro.
*/
int tsmn(int amount_tasks, const char *argv[]);

/* Inicializa o vetor de tarefas a partir dos arquivos de entrada. 

   Parâmetros:
   - arrayTasks: ponteiro para o vetor de tarefas a ser inicializado.
   - files_name: vetor de strings com os nomes dos arquivos de tarefas.
   - amount_tasks: o número de tarefas a serem inicializadas.
 
   Retorno:
  - Retorna 0 se as tarefas forem inicializadas com sucesso. Caso contrário, retorna um código de erro.
*/
int initializeArrayTasks(Tasks *arrayTasks, const char *files_name[], int amount_tasks);

/* Conta as instruções de uma tarefa.

   Parâmetros:
   - task: referência à tarefa para a qual as instruções serão contadas.
*/
void countInstructions(Tasks *task);

/* Armazena os endereços das páginas de memória da tarefa.
 
   Parâmetros:
   - task: referência à tarefa cujos endereços de memória serão armazenados.
   - memoryProgram: um ponteiro para a estrutura que mantém o controle da memória física do programa.
*/
void storeTaskAddresses(Tasks *task, MemoryProgram *memoryProgram);

/* Inicializa o escalonador e as filas de tarefas.

   Parâmetros:
   - round_robin: ponteiro para a estrutura do escalonador que será inicializada.
   - arrayTasks: vetor de tarefas que o escalonador utilizará.
   - amount_tasks: número de tarefas para o escalonador gerenciar.
   - program: ponteiro para a estrutura que armazena os tempos de execução do programa.
   - memoryProgram: ponteiro para a estrutura de memória física do programa.
 
   Retorno:
   - Retorna 0 se a inicialização for bem-sucedida. Caso contrário, retorna um código de erro.
 */
int initializeScheduler(Scheduler *round_robin, Tasks *arrayTasks, int amount_tasks, TimeProgram *program, MemoryProgram *memoryProgram);

/* Simuula a execução do escalonador round-robin para gerenciar a execução das tarefas.
 
   Parâmetros:
   - scheduler: ponteiro para o escalonador que gerencia as filas de tarefas.
   - terminatedQueue: fila onde as tarefas terminadas serão enfileiradas.
   - program: estrutura que mantém os tempos totais de execução do programa.
   - memoryProgram: estrutura que mantém a memória física do programa.

   Retorno:
   - Retorna 0 se a execução for bem-sucedida. Caso contrário, retorna um código de erro.
*/
int executeRoundRobin(Scheduler *scheduler, Queue *terminatedQueue, TimeProgram *program, MemoryProgram *memoryProgram);

/* Executa uma instrução de uma tarefa.
 
   Parâmetros:
   - task: referência à tarefa que executará a instrução.
   - memoryProgram: estrutura que mantém a memória física do programa.
   - timeProgram: estrutura que mantém o tempo total de execução do programa.
 
   Retorno:
   - Retorna 0 se a instrução for executada com sucesso. Caso contrário, retorna um código de erro.
*/
int executeInstruction(Tasks *task, MemoryProgram *memoryProgram, TimeProgram *timeProgram);

/* Valida a instrução de uma tarefa.
 
  Parâmetros:
   - task: referência à tarefa que contém a instrução a ser validada.
   - line: linha de texto representando a instrução a ser validada.
 
   Retorno:
   - Retorna 1 se a instrução for válida. Caso contrário, retorna 0.
*/
int validateInstructions(Tasks *task, const char *line);

/* Simula a alocação de memória para uma tarefa.

   Parâmetros:
   - task: referência à tarefa que realizará a alocação de memória.
   - line: string que contém o tamanho de memória a ser alocado.
 
   Retorno:
   - Retorna 0 se a alocação for bem-sucedida. Caso contrário, retorna um código de erro.
 */
void alocateMemory(Tasks *task, const char *line, MemoryProgram *memoryProgram);

/* Simula o acesso ao disco para uma tarefa.
 
   Parâmetros:
   - task: referência à tarefa que realizará o acesso ao disco.
   - timeProgram: estrutura que mantém o tempo total de execução do programa.
*/
void accessDisk(Tasks *task, TimeProgram *timeProgram); 

/* Armazena as instruções de uma tarefa.
 
   Esta função armazena as instruções de uma tarefa, associando cada instrução a um endereço de memória.
 
   Parâmetros:
   - task: referência à tarefa cujas instruções serão armazenadas.
   - memoryProgram: estrutura que mantém a memória física do programa.
 
   Retorno:
   - Retorna 0 se as instruções forem armazenadas com sucesso. Caso contrário, retorna um código de erro.
*/
void storeInstructions(MemoryProgram *memoryProgram, Queue *terminatedQueue); 

/* Armazena os arrays de uma tarefa.

   Esta função armazena os arrays de uma tarefa a partir de uma linha de entrada, criando os vetores conforme a solicitação.
 
   Parâmetros:
   - task: referência à tarefa cujos arrays serão armazenados.
   - line: linha de texto que contém as informações sobre os arrays.
 
   Retorno:
   - Retorna 0 se os arrays forem armazenados com sucesso. Caso contrário, retorna um código de erro.
*/
int storeArrays(Tasks *task, const char *line); 

/* Verifica se a tarefa pode ser executada.
 
  Parâmetros:
 - task: referência à tarefa a ser validada.
 
  Retorno:
  - Retorna 1 se a tarefa for válida e pode ser executada. Caso contrário, retorna 0.
*/
int validateTask(Tasks *task); // Valida se a tarefa pode ser executada.

/* Abre um arquivo para leitura.
   
   Parâmetros:
   - name_file: nome do arquivo a ser aberto.
 
   Retorno:
 - Retorna um ponteiro para o arquivo aberto, ou NULL em caso de erro ao abrir o arquivo.
 */
FILE* openFile(const char *name_file); // Abre um arquivo para leitura.

/* Cria uma nova fila de tarefas.

   Retorno:
   - Retorna ponteiro para a nova fila de tarefas.
*/
Queue* createQueueTasks(); // Cria uma nova fila de tarefas.

/* Enfileira uma tarefa.

   Parâmetros:
   - queue: ponteiro para a fila onde a tarefa será enfileirada.
   - task: tarefa a ser enfileirada.
 
   Retorno:
   - Retorna 0 se a tarefa foi enfileirada com sucesso. Caso contrário, retorna um código de erro.
 */
int enqueue(Queue *queue, Tasks task); // Enfileira uma tarefa na fila.

/* Desenfileira uma tarefa da fila.
  
   Parâmetros:
   - queue: ponteiro para a fila de onde a tarefa será removida.
   - task: ponteiro para a estrutura onde a tarefa desenfileirada será armazenada.
 
   Retorno:
   - Retorna 0 se a tarefa foi desenfileirada com sucesso. Caso contrário, retorna um código de erro.
 */
int dequeue(Queue *queue, Tasks *task); // Desenfileira uma tarefa da fila.


/* Libera a memória da fila de tarefas.
  
   Parâmetros:
   - queue: ponteiro para a fila que será liberada.
*/
void freeQueueTasks(Queue *queue); 

/* Cria a tabela de páginas de memória para uma tarefa.
  
   Parâmetros:
   - task: referência à tarefa que terá sua tabela de páginas criada.
*/
void createPageTable(Tasks *task, MemoryProgram *MemoryProgram);  

/* Gera o relatório de execução das tarefas.
  
   Parâmetros:
   - terminatedQueue: fila contendo as tarefas terminadas.
   - program: estrutura que mantém o tempo de execução do programa.
   - amount_files: número total de arquivos de tarefas processados.
*/
void printReport(Queue *terminatedQueue, TimeProgram timeProgram, MemoryProgram *memoryProgram, int amount_files); 

/* Imprime informações sobre uma tarefa.

   Parâmetros:
   - current: ponteiro para o nó que contém a tarefa cujas informações serão impressas.
   - program: estrutura que mantém o tempo total de execução do programa.
*/
void printTask(Node *current, TimeProgram program); 

/* Imprime as instruções de uma tarefa.

   Parâmetros:
   - task: tarefa cujas instruções serão impressas.
*/
void printInstructions(Tasks task); 

/* Imprime a tabela de páginas de memória de uma tarefa.
  
   Parâmetros:
  - task: tarefa cujas páginas de memória serão impressas.
*/
void printPageTable(Tasks task); 
