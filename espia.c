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
    Definition of the state
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
    int shmsize;
    int shmid;
    int *mapSize;
    int shmarray;
    int shmarraysize;
    int *sizeArray;
    int *array;
    int type;
	
    while (1)
    {
        printf("\nSelect type of consultation\n");
        printf("1. Memory state\n");
        printf("2. Processes states\n");
        printf("3. Exit\n");
        scanf("%d", &type); // Ask for shared memory size from user
        printf("\n");

        if (type == 2)
        {
            shmarraysize = shmget(keyStructSize, sizeof(int), IPC_CREAT | 0666); // Get shared memory size
            sizeArray = (int *)shmat(shmarraysize, 0, 0);
            SIZE = sizeArray[0];
			
			if(SIZE == 0){
				printf("Please create a shared memory with a size greater than 0.\n\n");
				return 0;
			}

            shmarray = shmget(keyStruct, SIZE * sizeof(int), IPC_CREAT | 0666); // Get shared memory size
            array = (int *)shmat(shmarray, 0, 0);

            for (int i = 0; i < SIZE+1; i++)
            {
				printf("Id: %d, ", i);
				if(array[i] == 0) printf("State: blocked\n");
				else if(array[i] == 1) printf("State: searching in memory\n");
                else if(array[i] == 2) printf("State: in memory\n");
				else if(array[i] == 3) printf("State: death\n");
				else if(array[i] == 4) printf("State: already done\n");
				//printf("State: %d, Id: %d\n", array[i], i);
            }
            shmdt((void *)sizeArray); // Detach memory space
            shmdt((void *)array);     // Detach memory space
			printf("\n");
        }
        else if (type == 1)
        {
            shmsize = shmget(keySize, sizeof(int), IPC_CREAT | 0666); // Get shared memory size

            mapSize = (int *)shmat(shmsize, 0, 0);

            printf("Memory size: %d \n\n", mapSize[0]);

            shmid = shmget(key, mapSize[0] * sizeof(int), IPC_CREAT | 0666); // Get shared memory
            int *array = (int *)shmat(shmid, 0, 0);                          // Map memory to arr

            printf("| ");
            for (int i = 0; i < mapSize[0]; i++)
            {
                printf("%d | ", array[i]);
            }
            printf("\n\n");

            //shmdt((void *)mapSize); // Detach memory space
            //shmdt((void *)array);   // Detach memory space
        }
        else if (type == 3)
        {
			exit(0);
            return 0;
        }
    }
}

