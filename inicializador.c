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
sem_t mutex;

int counterGlobal = 0; // Counter for PID of processes, would be managed by process structure
int segSize;           // This would be generated randomly by the init function
int SIZE;              // Size of shared memory, given by user input in init function

int main()
{
    // INIT FUNCTION
    int shmid;
    printf("Enter shared memory size: ");
    scanf("%d", &SIZE); // Ask for shared memory size from user
    printf("\n\n");                                 // initilalize semaphore
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Create shared memory space
    int *array = (int *)shmat(shmid, 0, 0);                    // Map memory space to array
    for (int pos = 0; pos < SIZE; pos++)                       // Initialize empty memory space with -1
    {
        array[pos] = -1;
    }
    // END OF INIT FUNCTION

    return 0;
}