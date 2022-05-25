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

int SIZE; // Size of shared memory, given by user input in init function

int main()
{
    // INIT FUNCTION
    int shmid;
    int shmidDoc;
    int shmsize;
    int *mapSize;

    // Write file
    FILE *files = fopen("bitacora.txt", "w");

    if (!files)
    {
        printf("Could not create file.\n");
        exit(EXIT_FAILURE);
    }
    // End File memory
	
	shmsize = shmget(keySize, sizeof(int), IPC_CREAT | 0666); // Create shared memory space by size
    mapSize = (int *)shmat(shmsize, 0, 0);                    // Map shared memory space to array
    if (mapSize[0] != 0){
		printf("\nA shared memory already exists.\n\n");
		return 0;
	}

    // Process memory
    printf("\nEnter shared memory size (please write it in numbers): ");
    scanf("%d", &SIZE);                                        // Ask for shared memory size from user       
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Create shared memory space
    int *array = (int *)shmat(shmid, 0, 0);                    // Map memory space to array
    for (int pos = 0; pos < SIZE; pos++)                       // Initialize empty memory space with -1
    {
        array[pos] = -1;
    }

    shmsize = shmget(keySize, sizeof(int), IPC_CREAT | 0666); // Create shared memory space by size
    mapSize = (int *)shmat(shmsize, 0, 0);                    // Map shared memory space to array
    mapSize[0] = SIZE;
	printf("Shared memory was created.\n\n");

    return 0;
}
