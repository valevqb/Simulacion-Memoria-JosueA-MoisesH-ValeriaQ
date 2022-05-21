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


key_t keyStruct = 1122233;
key_t keyStructSize = 44332211;
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
    int shmarray;
    int shmarraysize;
    int* sizeArray;
    int* array;


    shmarraysize = shmget(keyStructSize, sizeof(int), IPC_CREAT | 0666); // Get shared memory size
	sizeArray = (int *)shmat(shmarraysize, 0, 0);
	SIZE = sizeArray[0];

    shmarray = shmget(keyStruct, SIZE * sizeof(int), IPC_CREAT | 0666); // Get shared memory size
	array = (int *)shmat(shmarray, 0, 0);

    shmdt((void *)sizeArray); // Detach memory space
	for (int i = 0; i < SIZE; i++)
    {
        printf("State: %d, Id: %d\n", array[i], i);
    }
    

    


    return 0;
}
