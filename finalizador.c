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

int SIZE = 10;              // Size of shared memory, given by user input in init function

int main()
{
    // INIT FUNCTION
    int shmid;
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Create shared memory space
    // Liberate shared memory space, this would be done by the process finalizer
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}