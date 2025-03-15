#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "tsmm.h"

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf(ERR_NO_PARAMETER, argv[0]);
        return EXIT_FAILURE;
    }
    else if (argc > MAX_TASKS + 1) {
        printf(ERR_MAX_PARAMETER);
        return EXIT_FAILURE;
    }
    return tsmn(argc - 1, (const char **)(argv + 1));
}

int tsmn(int amount_tasks, const char *files_name[]) {
    setlocale(LC_ALL, LOCALE);
    
    // Inicializa o vetor de tarefas.
    Tasks arrayTasks[MAX_TASKS];
    initializeArrayTasks(arrayTasks, files_name, amount_tasks);

    // Inicializa o escalonador.
    Scheduler round_robin;
    TimeProgram timeProgram;
    MemoryProgram memoryProgram;
    if (!initializeScheduler(&round_robin, arrayTasks, amount_tasks, &timeProgram, &memoryProgram))
        return EXIT_FAILURE;

    // Cria fila, onde serão armazenadas as tarefas prontas.
    Queue *terminatedQueue = createQueueTasks();
    if (!terminatedQueue) 
        return EXIT_FAILURE;

    // Executa o escalonador até seu trabalho terminar.
    while (executeRoundRobin(&round_robin, terminatedQueue, &timeProgram, &memoryProgram));

    // Armazena as instruções e seus endereços.
    storeInstructions(&memoryProgram, terminatedQueue);

    printReport(terminatedQueue, timeProgram, &memoryProgram, amount_tasks);
    freeQueueTasks(terminatedQueue);
    return EXIT_SUCCESS;
}

int initializeArrayTasks(Tasks *arrayTasks, const char *files_name[], int amount_tasks) {
    for (int i = 0; i < amount_tasks; i++) {
        // Inicializa os valores dos vetores e outras variáveis.
        arrayTasks[i].status = READY;
        arrayTasks[i].ut = 0;
        arrayTasks[i].file = NULL;
        arrayTasks[i].arrayCount = 0;
        arrayTasks[i].memoryTask.allocatedDate = 0;
        arrayTasks[i].timeTask.timeEnteredQueue = i;
        arrayTasks[i].timeTask.waitTimeTask = 0;

        // Copia o nome do arquivo para a estrutura, garantindo a terminação nula.
        strncpy(arrayTasks[i].name, files_name[i], MAX_NAME - 1);
        arrayTasks[i].name[MAX_NAME - 1] = '\0';
        
        // Adiciona um ponteiro para os arquivos das tarefas.
        arrayTasks[i].file = openFile(files_name[i]);  
        if (!arrayTasks[i].file) 
            return 0;
        
        // Adiciona a quantidade de instruções da tarefa à sua própria estrutura.
        countInstructions(&arrayTasks[i]);
    }
    return 1;
}

void countInstructions(Tasks *task) {
    int instructions = 0;
    char line[MAX_LINE];

    while (fgets(line, MAX_LINE, task->file)) instructions++;
    
    // Diminui para desconsiderar a primeira linha, que não é uma instrução.
    task->instructionCount = instructions - 1;
    
    // Atualiza o tempo que sera gasto de cpu para a execução da tarefa.
    task->timeTask.cpuTimeTask = task->instructionCount;
    
    rewind(task->file);
}

int initializeScheduler(Scheduler *round_robin, Tasks *arrayTasks, int amount_tasks, TimeProgram *timeProgram, MemoryProgram *memoryProgram) {
    // Inicializa as variáveis de controle do tempo gasto pelo progama.
    timeProgram->waitingTime = 0;
    timeProgram->turnaroundTime = 0;
    memoryProgram->lastPhysicalAddress = BEGIN_TASKS_ADDRESS;

    // Cria fila de tarefas prontas, cada nodo é uma tarefa (arquivo).
    round_robin->readyQueue = createQueueTasks();
    if (!round_robin->readyQueue) return 0;
    
    // Cria fila para armazenar as tarefas suspensas.
    round_robin->suspendedQueue = createQueueTasks();
    if (!round_robin->suspendedQueue) return 0;

    for (int i = 0; i < amount_tasks; i++) {
        switch (validateTask(&arrayTasks[i])) { // Apenas adiciona arquivos válidos na fila.
            case NOT_OPEN_FILE: 
                printf(ERR_OPEN_FILE, arrayTasks[i].name);
                continue; 
            case EMPTY_FILE: 
                printf(ERR_FILE_EMPTY, arrayTasks[i].name);
                continue; 
            case INVALID_FILE:
                printf(ERR_INVLID_FILE, arrayTasks[i].name);
                continue; 
        }

        // Armazena a quantidade de páginas de código e a quantidade total de páginas. Logo após saber se ela é válida.
        storeTaskAddresses(&arrayTasks[i], memoryProgram); 

        Node *node = (Node*)calloc(1, sizeof(Node));
        if (!node) return 0;

        node->task = arrayTasks[i];
        node->task.status = READY;

        // Arquivo válido é inserido na fila de aptos.
        enqueue(round_robin->readyQueue, node->task); 
    }
    return 1;
}

void storeTaskAddresses(Tasks *task, MemoryProgram *memoryProgram) {
    if (memoryProgram->lastPhysicalAddress < BEGIN_TASKS_ADDRESS) 
        memoryProgram->lastPhysicalAddress = BEGIN_TASKS_ADDRESS; // Garante início correto.

    task->memoryTask.usedCodePages = (task->memoryTask.allocatedMemory + PAGE_SIZE - 1) / PAGE_SIZE;
    task->memoryTask.totalPages = task->memoryTask.usedCodePages;
    
    // Atualiza a memória física para a próxima tarefa.
    int startPhysicalPage = memoryProgram->lastPhysicalAddress / PAGE_SIZE;
    memoryProgram->lastPhysicalAddress = (startPhysicalPage + task->memoryTask.totalPages) * PAGE_SIZE;
}

int executeRoundRobin(Scheduler *scheduler, Queue *terminatedQueue, TimeProgram *timeProgram, MemoryProgram *memoryProgram) {
        
    int executedInstructions = 0; // Para controle do quantum.
    
    // Enquanto houver tarefas prontas ou suspensas.
    while (scheduler->readyQueue->size > 0 || scheduler->suspendedQueue->size > 0) {
        
        // Atualiza tarefas suspensas.
        Node *currentSuspended = scheduler->suspendedQueue->front;
        while (currentSuspended) {
        
            if (executedInstructions == 0) executedInstructions = 1; // Evita loop.
            
            // Decrementa a quantidade exata de terafas executadas.
            currentSuspended->task.timeTask.remainingTimeIO -= executedInstructions; 
            
            // Retorna as tarefas à fila de aptos após o tempo de espera terminar.
            if (currentSuspended->task.timeTask.remainingTimeIO <= 0) { 
                Tasks taskToReady;
                dequeue(scheduler->suspendedQueue, &taskToReady);
                taskToReady.status = READY;
                enqueue(scheduler->readyQueue, taskToReady);
            }
            currentSuspended = currentSuspended->next;
        }

        // Executa a próxima tarefa pronta.
        if (scheduler->readyQueue->size > 0) {

            Tasks currentTask;
            dequeue(scheduler->readyQueue, &currentTask);
            currentTask.status = RUNNING;

            Node *aux = scheduler->readyQueue->front;
            while (aux) {
                aux->task.timeTask.waitTimeTask++;
                timeProgram->waitingTime++;
                aux = aux->next;
            }
            
            int instructionResult;
            executedInstructions = 0; // Reinicia a contagem.

            while (executedInstructions < QUANTUM_UT) {

                instructionResult = executeInstruction(&currentTask, memoryProgram, timeProgram);
                
                if (instructionResult == TERMINATED) { // Arquivo chegou ao fim e a terefa é finalizada.
                    currentTask.status = COMPLETED;
                    fclose(currentTask.file);
                    currentTask.file = NULL;
                    enqueue(terminatedQueue, currentTask);
                    break; 
                }
                if (instructionResult == INVALID) { // Decrementa os valores da tarefa inválida.
                    currentTask.status = INVALID;
                    timeProgram->turnaroundTime -= currentTask.ut;
                    timeProgram->waitingTime -= currentTask.timeTask.waitTimeTask;
                    break; // "Perde-se" a tarefa inválida, para não ser imprimida no relatório. 
                }
                if (instructionResult == DISK_ACCESS) { // Suspensão por I/O.
                    currentTask.status = SUSPENDED;
                    executedInstructions++;
                    timeProgram->turnaroundTime += UT;
                    enqueue(scheduler->suspendedQueue, currentTask);
                    break;
                }
                else {
                    currentTask.ut += UT;
                    executedInstructions++;
                    timeProgram->turnaroundTime += UT;
                }
            }

            if (currentTask.status == RUNNING) {
                // Tarefa não terminou, volta para fila de prontas.
                enqueue(scheduler->readyQueue, currentTask);
            }
        }
    }
    freeQueueTasks(scheduler->readyQueue);
    freeQueueTasks(scheduler->suspendedQueue);
    return 0;
}

int executeInstruction(Tasks *task, MemoryProgram *memoryProgram, TimeProgram *timeProgram) {
    char line[MAX_LINE]; // Buffer para armazenar a linha atual.

    // Certifica de que o arquivo está aberto.
    if (!task->file) {
        task->file = openFile(task->name);
        
        if (!task->file) return INVALID; // Erro ao abrir o arquivo.

        fgets(line, MAX_LINE, task->file); // Pula a primeira linha, que não é uma instrução.
    }

    // Lê a próxima linha.
    if (!fgets(line, MAX_LINE, task->file)) { 
        // Verifica se chegou ao fim do arquivo.
        if (feof(task->file)) {
            return TERMINATED; // Tarefa concluída.
        }
        return INVALID; // Erro de leitura.
    }

    // Valida e executa a instrução.
    switch (validateInstructions(task, line)) {
        case INVALID_TASK:
            printf(ERR_TASK_INVALID, task->name);
            return INVALID; // Erro de instrução desconhecida.

        case INVALID_ACCESS:
            return INVALID; // Erro de acesso a memória.

        case INVALID_ALLOCATION:
            return INVALID; // Erro na alocação inicial para a terefa.
        
        case MEMORY_ALLOCATION:
            alocateMemory(task, line, memoryProgram);
            storeArrays(task, line);
            return MEMORY_ALLOCATION; 
        
        case MEMORY_ACCESS:
            return MEMORY_ACCESS;
        
        case DISK_ACCESS:
            accessDisk(task, timeProgram);
            return DISK_ACCESS;
    }
}

int storeArrays(Tasks *task, const char *line) {
    int num;
    char arrayName[MAX_NAME];

    // Usa %[^ ] para capturar tudo até o primeiro espaço em branco.
    if (sscanf(line, "%s new %d", arrayName, &num) == 2) {

        // Armazena o nome do vetor criado na próxima posição disponível.
        if (task->arrayCount < MAX_INSTRUCTIONS) { // Verifica se não ultrapassa o limite.
            strncpy(task->arrays[task->arrayCount], arrayName, MAX_NAME);
            task->arraysSize[task->arrayCount] = num;
            task->arrayCount++; // Incrementa o índice para o próximo vetor.
        } 
        else {
            printf(ERR_MAX_ARRAYS, MAX_INSTRUCTIONS); 
            return INVALID;
        }
    } else {
        printf(ERR_PARSE_LINE, line); 
        return INVALID;
    }
    return 1; 
}

int validateInstructions(Tasks *task, const char *line) { 

    // Verifica se "new" está presente na linha.
    if (strstr(line, NEW) != NULL) { 
        int size;
        // Extrai o valor de "new n".
        sscanf(line, "%*s new %d", &size);

        // Verifica se há memória disponível dentro do limite da tarefa.
        if (task->memoryTask.allocatedMemory + size > MAX_MEMORY_TASK) {
            printf(ERR_ALOCATE_MEMORY, task->name, MAX_MEMORY_TASK);
            return INVALID_ALLOCATION; // Falha na "alocação".
        }
        return MEMORY_ALLOCATION; // Código para a instrução 'NEW'.
    }

    // Verifica "v[p]".
    const char *start = strchr(line, '[');
    const char *end = strchr(line, ']');
    if (start && end && end > start + 1) {
        char arrayName[MAX_NAME];
        int index;

        // Extrai o nome do vetor e o índice.
        if (sscanf(line, "%[^[][%d]", arrayName, &index) == 2 && index >= 0) {

            // Percorre os vetores armazenados em arrays para verificar se o vetor existe.
            for (int i = 0; i < task->arrayCount; i++) {
                if (strcmp(task->arrays[i], arrayName) == 0) {

                    // Verifica se o índice desejado é válido
                    if (index >= 0 && index < task->arraysSize[i]) {
                        return MEMORY_ACCESS; // Instrução "v[p]" válida.
                    } 
                    else {
                        printf(ERR_ACCESS_MEMORY, task->name, index);
                        return INVALID_ACCESS;
                    }
                }
            }
            printf(ERR_ACCESS_ARRAY, arrayName);
            return INVALID; // Vetor não encontrado.
        }
    }
    
    if (strncmp(line, READ, strlen(READ)) == 0) return DISK_ACCESS;
        
    return INVALID_TASK; // Instrução diferente das permitidas.
}

void alocateMemory(Tasks *task, const char *line, MemoryProgram *memoryProgram) { 
    int size;
    
    // Extrai o valor de "new n".
    sscanf(line, "%*s new %d", &size);

    // Atualiza a memória total alocada pela tarefa.
    task->memoryTask.allocatedMemory += size;

    // Atualiza a memória de dados alocada pela tarefa.
    task->memoryTask.allocatedDate += size;

    // Calcula a nova quantidade de páginas de código e dados.
    int newUsedDataPages = (task->memoryTask.allocatedDate + PAGE_SIZE - 1) / PAGE_SIZE;
    
    if (newUsedDataPages == 0) newUsedDataPages = 1;
    
    // Atualiza a quantidade total de páginas alocadas e a de dados, caso a quantidade de páginas de dados tenha aumentado.
    if (newUsedDataPages > task->memoryTask.usedDataPages) {
        task->memoryTask.totalPages++;
        task->memoryTask.usedDataPages = newUsedDataPages;
    }
}

void accessDisk(Tasks *task, TimeProgram *timeProgram) {
    // Adiciona o tempo que a tarefa deve esperar para poder voltar ao status de apto.
    task->timeTask.remainingTimeIO = IO_WAIT_TIME; 
    
    // Incrementa o tempo total gasto em E/S da terfa.
    task->timeTask.IOtimeTask += IO_WAIT_TIME; 

    // Incrementa o tempo total gasto em E/S do programa.
    timeProgram->IOTime += IO_WAIT_TIME;
}

// Só é chamado quando todas as instruções do arquivo são válidas. 
void storeInstructions(MemoryProgram *memoryProgram, Queue *terminatedQueue) {
    for (int i = 0; i < terminatedQueue->size; i++) {
        Node *current = terminatedQueue->front;
        while (current) {
            if (current->task.timeTask.timeEnteredQueue == i) {
                FILE *f = openFile(current->task.name);

                char instruction[MAX_LINE];      
                int firstDataPage = current->task.memoryTask.usedCodePages; 
                int logicalAddress = firstDataPage * PAGE_SIZE;  
                int index = 0;

                // Pula a primeira linha
                fgets(instruction, sizeof(instruction), f);

                // Define o primeiro endereço físico disponível após as páginas de código
                int nextPhysicalAddress = (MAX_PROGRAM_PAGES + firstDataPage) * PAGE_SIZE;
                nextPhysicalAddress += current->task.memoryTask.usedCodePages * PAGE_SIZE;


                while (fgets(instruction, sizeof(instruction), f)) {
                    instruction[strcspn(instruction, "\n")] = '\0';
                    strncpy(current->task.instructionsTask[index].nameInstructions, instruction, MAX_LINE - 1);

                    int size = 0;
                    int instructionType = validateInstructions(&current->task, instruction);

                    if (instructionType == MEMORY_ALLOCATION) { 
                        sscanf(instruction, "%*s new %d", &size);
                        int previousLogicalAddress = logicalAddress;
                        logicalAddress += size;

                        int logicalPage = previousLogicalAddress / PAGE_SIZE;
                        int logicalOffset = previousLogicalAddress % PAGE_SIZE;
                        
                        int physicalPage = nextPhysicalAddress / PAGE_SIZE;
                        int physicalOffset = nextPhysicalAddress % PAGE_SIZE;

                        current->task.instructionsTask[index].adressInstruction.beginLogicalAddress = previousLogicalAddress;
                        current->task.instructionsTask[index].adressInstruction.endLogicalAddress = logicalAddress - 1;
                        current->task.instructionsTask[index].adressInstruction.beginPhysicalAddress = nextPhysicalAddress;
                        current->task.instructionsTask[index].adressInstruction.endPhysicalAddress = nextPhysicalAddress + size - 1;

                        current->task.instructionsTask[index].adressInstruction.page[LOGICAL] = logicalPage;
                        current->task.instructionsTask[index].adressInstruction.offset[LOGICAL] = logicalOffset;
                        current->task.instructionsTask[index].adressInstruction.page[PHYSICAL] = physicalPage;
                        current->task.instructionsTask[index].adressInstruction.offset[PHYSICAL] = physicalOffset;

                        // Atualiza o próximo endereço físico.
                        nextPhysicalAddress += size;
                    } 
                    else if (instructionType == MEMORY_ACCESS) {
                        char arrayName[MAX_NAME];
                        int position;
                        int beginLogical = -1;
                        int beginPhysical = -1;

                        sscanf(instruction, "%[^[][%d]", arrayName, &position);

                        for (int i = 0; i < current->task.arrayCount; i++) {
                            if (strcmp(current->task.arrays[i], arrayName) == 0) {
                                beginLogical = current->task.instructionsTask[i].adressInstruction.beginLogicalAddress;
                                beginPhysical = current->task.instructionsTask[i].adressInstruction.beginPhysicalAddress;
                                break;
                            }
                        }

                        if (beginLogical == -1) {
                            printf(ERR_ACCESS_ARRAY, arrayName);
                            continue;
                        }

                        int logicalPage = (beginLogical + position) / PAGE_SIZE;
                        int logicalOffset = (beginLogical + position) % PAGE_SIZE;
                        int physicalPage = (beginPhysical + position) / PAGE_SIZE;
                        int physicalOffset = (beginPhysical + position) % PAGE_SIZE;

                        current->task.instructionsTask[index].adressInstruction.page[LOGICAL] = logicalPage;
                        current->task.instructionsTask[index].adressInstruction.offset[LOGICAL] = logicalOffset;
                        current->task.instructionsTask[index].adressInstruction.page[PHYSICAL] = physicalPage;
                        current->task.instructionsTask[index].adressInstruction.offset[PHYSICAL] = physicalOffset;
                    }

                    index++;
                }
                
                fclose(f);
            }
            current = current->next;
        }
    }
}

// Além de validar a tarefa, analisa e aloca o tamanho necessário inicial da terfa. 
int validateTask(Tasks *task) { 
    if (!task->file) return NOT_OPEN_FILE; // Verifica se o arquivo foi aberto corretamente.

    char line[MAX_LINE]; // Buffer para armazenar a linha lida.
    rewind(task->file);  // Garante que a leitura começa do início do arquivo.

    // Lê a primeira linha do arquivo.
    if (fgets(line, MAX_LINE, task->file)) {
        // Verifica se a linha começa com "#T="
        if (strncmp(line, SIZE, strlen(SIZE)) == 0) {
            int memorySize;
            
            // Extrai o número após "#T="
            if (sscanf(line + strlen(SIZE), "%d", &memorySize) == 1) {
                // Armazena a quantidade de memória inicial requerida.
                task->memoryTask.allocatedMemory = memorySize; 
                task->memoryTask.allocatedCode = memorySize;

                // Verifica se a memória alocada não ultrapassa o limite permitido.
                if (memorySize <= MAX_MEMORY_TASK) 
                    return 1; // Memória válida.
                else 
                    return 0; // Memória inválida.
            }
        }
        return INVALID_FILE; // Arquivo inválido se não contiver "#T=" corretamente formatado.
    }
    return EMPTY_FILE; // Arquivo vazio.
}

void createPageTable(Tasks *task, MemoryProgram *memoryProgram) {
    // Define a primeira página física disponível para o programa.
    static int nextPhysicalPage = MAX_PROGRAM_PAGES; 

    // Verifica a quantidade de páginas de código usadas pela tarefa.
    task->memoryTask.usedCodePages = (task->memoryTask.allocatedCode + PAGE_SIZE - 1) / PAGE_SIZE;

    // Armazena a tabela de páginas lógicas e físicas para as páginas de código.
    for (int i = 0; i < task->memoryTask.usedCodePages; i++) {
        // Páginas.
        task->memoryTask.pagesTable[i].page[LOGICAL] = i;              
        task->memoryTask.pagesTable[i].page[PHYSICAL] = nextPhysicalPage;
        
        // Endereços.
        task->memoryTask.pagesTable[i].beginLogicalAddress = i * PAGE_SIZE;
        task->memoryTask.pagesTable[i].endLogicalAddress = (i + 1) * PAGE_SIZE - 1;
        task->memoryTask.pagesTable[i].beginPhysicalAddress = nextPhysicalPage * PAGE_SIZE;
        task->memoryTask.pagesTable[i].endPhysicalAddress = (nextPhysicalPage + 1) * PAGE_SIZE - 1;
        
        // Atualiza a próxima página física.
        nextPhysicalPage++;        
    }
    
    // Armazena a tabela de páginas lógicas e físicas para as páginas de dados.
    for (int i = 0; i < task->memoryTask.usedDataPages; i++) {
        // Páginas.
        int logicalPageIndex = task->memoryTask.usedCodePages + i;
        task->memoryTask.pagesTable[logicalPageIndex].page[LOGICAL] = logicalPageIndex;
        task->memoryTask.pagesTable[logicalPageIndex].page[PHYSICAL] = nextPhysicalPage;
        
        // Endereços.
        task->memoryTask.pagesTable[logicalPageIndex].beginLogicalAddress = logicalPageIndex * PAGE_SIZE;
        task->memoryTask.pagesTable[logicalPageIndex].endLogicalAddress = (logicalPageIndex + 1) * PAGE_SIZE - 1;
        task->memoryTask.pagesTable[logicalPageIndex].beginPhysicalAddress = nextPhysicalPage * PAGE_SIZE;
        task->memoryTask.pagesTable[logicalPageIndex].endPhysicalAddress = (nextPhysicalPage + 1) * PAGE_SIZE - 1;

        // Atualiza a próxima página física.
        nextPhysicalPage++;            
    }

    // Após a alocação de páginas, atualiza o endereço físico da última página alocada.
    memoryProgram->lastPhysicalAddress = nextPhysicalPage * PAGE_SIZE;
}

FILE* openFile(const char *name_file) {
    char full_name[MAX_NAME];
    snprintf(full_name, sizeof(full_name), "%s.tsk", name_file); // Concatena o nome do arquivo.

    FILE *f = fopen(full_name, "r");
    if (!f) 

    return f;
}

Queue* createQueueTasks() {
    Queue *queue = (Queue*)calloc(1, sizeof(Queue));
    if (queue) {
        queue->front = queue->back = NULL;
        queue->size = 0;
    }    
    return queue;
} 

int enqueue(Queue *queue, Tasks task) {
    Node *node = (Node*)calloc(1, sizeof(Node));
    if (!node) return 0;
    
    node->task = task;
        
    if (!queue->size) {
        queue->front = queue->back = node;
    }
    else {
        queue->back->next = node;
        queue->back = node;
    }
    queue->size++;
    return 1;
}

int dequeue(Queue *queue, Tasks *task) {
    if (!queue->size) return 0;

    *task = queue->front->task;
    Node *aux = queue->front;
    queue->front = queue->front->next;
    free(aux);
    queue->size--;
    return 1;
}

void freeQueueTasks(Queue *queue) {
    while (queue->front) {
        Node *aux = queue->front;
        queue->front = queue->front->next;
        free(aux);
    }
    free(queue);
}

void printReport(Queue *terminatedQueue, TimeProgram timeProgram, MemoryProgram *memoryProgram, int amount_files){
    
    printf("\n\n- Round-Robin");
    printf("\n\tTempo médio de execução = %.2f ut", timeProgram.turnaroundTime / terminatedQueue->size);
    printf("\n\tTempo médio de espera = %.2f ut", timeProgram.waitingTime / terminatedQueue->size);

    for (int i = 0; i < amount_files; i++) {
        Node *current = terminatedQueue->front;
        while (current) {
            if (current->task.timeTask.timeEnteredQueue == i) {
                createPageTable(&current->task, memoryProgram);
                printTask(current, timeProgram);        
            }
            current = current->next;
        }
    }
    printf("\n\n");
}

void printTask(Node *current, TimeProgram program) {

    printf("\n\n- Tarefa: %s", current->task.name);

    printf("\n\t- CPU e Disco");
    printf("\n\t\tTempo de CPU = %d ut", current->task.timeTask.cpuTimeTask); 
    printf("\n\t\tTempo de E/S = %d ut", current->task.timeTask.IOtimeTask);    

    if (program.turnaroundTime) 
        printf("\n\t\tTaxa de ocupação da CPU = %.2f%%", (current->task.timeTask.cpuTimeTask / program.turnaroundTime) * 100);
    else 
        printf("\n\t\tTaxa de ocupação da CPU = %.2f%%", 0.0);

    if (program.waitingTime) 
        printf("\n\t\tTaxa de ocupação do disco = %.2f%%\n", (current->task.timeTask.IOtimeTask / program.IOTime) * 100);
    else 
        printf("\n\t\tTaxa de ocupação do disco = %.2f%%", 0.0);

    printf("\n\t- Memória");
    printf("\n\t\tNúmero de páginas lógicas = %d\n", current->task.memoryTask.totalPages);
    
    printInstructions(current->task);
    printPageTable(current->task);
}

void printInstructions(Tasks task) {
    int array = 0;
    
    for (int i = 0; i < task.instructionCount; i++) {
        int instructionType = validateInstructions(&task, task.instructionsTask[i].nameInstructions);

        if (instructionType == DISK_ACCESS) continue; // Não printa read disk.
        
        if (instructionType == MEMORY_ALLOCATION) {
            printf("\n\t\t- %s", task.arrays[array++]);

            printf("\n\t\t- Endereços Lógicos = %d a %d ( %d : %d a %d : %d )", 
                task.instructionsTask[i].adressInstruction.beginLogicalAddress,
                task.instructionsTask[i].adressInstruction.endLogicalAddress,
                task.instructionsTask[i].adressInstruction.page[LOGICAL], 
                task.instructionsTask[i].adressInstruction.beginLogicalAddress % PAGE_SIZE,
                task.instructionsTask[i].adressInstruction.page[LOGICAL], 
                task.instructionsTask[i].adressInstruction.endLogicalAddress % PAGE_SIZE);

            printf("\n\t\t- Endereços Físicos = %d a %d ( %d : %d a %d : %d )\n", 
                task.instructionsTask[i].adressInstruction.beginPhysicalAddress,
                task.instructionsTask[i].adressInstruction.endPhysicalAddress,
                task.instructionsTask[i].adressInstruction.page[PHYSICAL], 
                task.instructionsTask[i].adressInstruction.beginPhysicalAddress % PAGE_SIZE, 
                task.instructionsTask[i].adressInstruction.page[PHYSICAL], 
                task.instructionsTask[i].adressInstruction.endPhysicalAddress % PAGE_SIZE);
        }
        else if (instructionType == MEMORY_ACCESS) {
            printf("\n\t\t- %s", task.instructionsTask[i].nameInstructions);
            
            printf("\n\t\t- Endereço Lógico = %d : %d",
                task.instructionsTask[i].adressInstruction.page[LOGICAL], 
                task.instructionsTask[i].adressInstruction.offset[LOGICAL]);

            printf("\n\t\t- Endereço Físico = %d : %d\n", 
                task.instructionsTask[i].adressInstruction.page[PHYSICAL], 
                task.instructionsTask[i].adressInstruction.offset[PHYSICAL]);
        }
    }
}

void printPageTable(Tasks task) {    
    // Imprime as páginas da tarefa.
    printf("\n\t\t- Tabela de Páginas\n");
    for (int i = 0; i < task.memoryTask.totalPages; i++) 
        printf("\t\t\tPL %d (%d a %d) --> PF %d (%d a %d)\n",
            task.memoryTask.pagesTable[i].page[LOGICAL],
            task.memoryTask.pagesTable[i].beginLogicalAddress,
            task.memoryTask.pagesTable[i].endLogicalAddress,
            task.memoryTask.pagesTable[i].page[PHYSICAL],
            task.memoryTask.pagesTable[i].beginPhysicalAddress,
            task.memoryTask.pagesTable[i].endPhysicalAddress);
}
