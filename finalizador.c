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
    int shmid;
    int shmsize;
    int *mapSize;
    int *mapSizeStruct;
    int sizeStruct;

    int shmstruct;

    // struct PCB *process = malloc(sizeof(struct PCB));
    shmsize = shmget(keySize, sizeof(int), IPC_CREAT | 0666); // Get shared memory size
    mapSize = (int *)shmat(shmsize, 0, 0);
    SIZE = mapSize[0];

    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Create shared memory space

	if(SIZE == 0){
		printf("Please create a shared memory with a size greater than 0\n\n");
		return 0;
	}
	
    shmdt((void *)mapSize); // Detach memory space

    sizeStruct = shmget(keyStructSize, sizeof(int), IPC_CREAT | 0666);
    mapSizeStruct = (int *)shmat(sizeStruct, 0, 0);

    shmstruct = shmget(keyStruct, mapSizeStruct[0] * sizeof(int), IPC_CREAT | 0666);
    shmdt((void *)mapSizeStruct); // Detach memory space

    // Liberate shared memory space, this would be done by the process finalizer
    shmctl(shmid, IPC_RMID, NULL);
    shmctl(shmsize, IPC_RMID, NULL); // Detach memory space
    shmctl(sizeStruct, IPC_RMID, NULL);
    shmctl(shmstruct, IPC_RMID, NULL);

    return 0;
}

