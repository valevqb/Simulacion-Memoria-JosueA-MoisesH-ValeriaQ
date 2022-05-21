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

key_t key = 12345678;
key_t keySize = 987;
sem_t mutex;

int SIZE; // Size of shared memory, given by user input in init function
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

int main()
{
    // For memory
    int shmid;
    int shmsize;
    int *mapSize;

    // struct PCB *process = malloc(sizeof(struct PCB));
    shmsize = shmget(keySize, sizeof(int), IPC_CREAT | 0666); // Get shared memory size
    mapSize = (int *)shmat(shmsize, 0, 0);
    SIZE = mapSize[0];

    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Create shared memory space

    shmdt((void *)mapSize); // Detach memory space

    // Liberate shared memory space, this would be done by the process finalizer
    shmctl(shmid, IPC_RMID, NULL);
    shmctl(shmsize, IPC_RMID, NULL); // Detach memory space

    return 0;
}
