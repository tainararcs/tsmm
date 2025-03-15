# tsmm - Task Scheduling and Memory Management

## 📌 Descrição
O **tsmm** é um programa em C que simula o escalonamento de tarefas e a gerência de memória em um sistema computacional. Ele usa o algoritmo de escalonamento **Round-Robin** e gerencia a memória utilizando **paginação simples**.

## 🚀 Funcionalidades
- Escalonamento de tarefas utilizando **Round-Robin** (quantum = 2 UT).
- Gerenciamento de memória com **paginação simples**.
- Simulação de instruções de:
  - **Alocação de memória** (`new n`)
  - **Acesso à memória** (`v[p]`)
  - **Acesso ao disco** (`read disk`)
- Relatório detalhado com:
  - Tempo médio de execução e espera das tarefas.
  - Taxa de ocupação da CPU e do disco.
  - Mapeamento dos endereços lógicos e físicos das alocações de memória.

## 📂 Estrutura dos Arquivos
- `tsmm.c`: Código-fonte principal do programa.
- `tsmm.h`: Arquivo de cabeçalho.
- `tarefas/*.tsk`: Arquivos de entrada contendo as tarefas a serem executadas.

## 🖥️ Como Compilar e Executar
### 🔧 Compilação
Para compilar o programa, utilize o `gcc` no Linux:
```sh
gcc -Wall -o tsmm tsmm.c
