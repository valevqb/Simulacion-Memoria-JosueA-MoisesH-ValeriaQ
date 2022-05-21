#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>

key_t key = 11223344;
sem_t mutex;

int counterGlobal = 0; // Counter for PID of processes, would be managed by process structure
int segSize;           // This would be generated randomly by the init function
int SIZE;              // Size of shared memory, given by user input in init function

// Process PCN
struct PCB
{
    int pId;
    int state;
    /*
    Defintion of the state
    =>0-Block process
    =>1-Search memory
    =>2-Process in memory
    =>3-Death process
    =>4-Finish process
    */
};

// List nodes
struct Node
{
    struct Node *next;
    struct PCB process;
};

// Queue structure
struct Queue
{
    struct Node *first;
    struct Node *last;
};

// Inserts process into the queue
int insertProcess(struct Queue *q, struct PCB pcb)
{
    if (q->first == NULL)
    {
        q->first = (struct Node *)malloc(sizeof(struct Node));
        q->first->next = NULL;
        q->first->process = pcb;
        q->last = q->first;
    }
    else
    {
        struct Node *tmp = (struct Node *)malloc(sizeof(struct Node));
        tmp->next = NULL;
        tmp->process = pcb;
        q->last->next = tmp;
        q->last = tmp;
    }
    return 0;
}

struct Node *searchProcessById(struct Queue *q, int pId)
{
    struct Node *node = q->first;
    while (node != NULL)
    {
        if (node->process.pId == pId)
        {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

int main()
{
    // INIT FUNCTION
    int shmid;
    struct PCB process;

    struct Node *tmp = malloc(sizeof(struct Node));
    shmid = shmget(key, sizeof(tmp), IPC_CREAT | 0666); // Create shared memory space

    tmp = (struct Node *)shmat(shmid, 0, 0); // Map shared memory space to array
    process.pId = 0;
    process.state = 91;
    tmp->process = process;

    shmdt((void *)tmp); // Detach memory space
    // Liberate shared memory space, this would be done by the process finalizer
    // shmctl(shmid, IPC_RMID, NULL);

    return 0;
}