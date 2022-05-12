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

void segmentProcess(void *arg)
{

    int counterLocal = counterGlobal; // Save current PID counter locally
    counterGlobal++;                  // Increase global PID counter
    int shmid;
    int full = 0;     // Flag for memory availability ; 0 = space was not found, 1 = space was found
    int segCount = 0; // Counter to check if there is enough continuous space

    /*
     * Here is where the sync algorithm would be implemented
     * Before waiting at the semaphore, each process would check if it's their turn
     */

    sem_wait(&mutex);                                          // Send wait signal to semaphore when ready
    printf("-------Busca espacio %d-------\n", counterLocal);  // PRINT FOR TESTING PURPOSES
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get shared memory
    int *array = (int *)shmat(shmid, 0, 0);                    // Map memory to array
    // Loop to check for available memory
    for (int pos = 0; pos < SIZE; pos++) // Here, SIZE would be replaced by the amount of segments
    {
        if (array[pos] == -1) // If the memory space is available
        {
            segCount++;              // Increase segment count
            if (segCount == segSize) // If the segment count is equal to the required space
            {
                while (segCount != 0) // Fill the available space
                {
                    segCount--;
                    array[pos - segCount] = counterLocal;
                }
                full = 1; // Memory wasn't full
                break;
            }
        }
        else // If memory space isn't available at current index
        {
            segCount = 0; // Reset segment count
        }
    }
    printf("Arreglo actual %d\n", counterLocal); // PRINT FOR TESTING PURPOSES
    for (int pos = 0; pos < SIZE; pos++)         // Print memory space status, FOR TESTING PURPOSES
    {
        printf("%d ", array[pos]);
    }
    printf("\n"); // PRINT FOR TESTING PURPOSES

    if (full) // If there was memory available
    {
        // NOT IMPLEMENTED
        // Write to log relevant information
        printf("------Encontro espacio %d-----\n", counterLocal); // PRINT FOR TESTING PURPOSES
    }
    else // If there was no memory available
    {
        // NOT IMPLEMENTED
        // Write to log relevant information
        printf("----No encontro espacio %d----\n", counterLocal); // PRINT FOR TESTING PURPOSES
        shmdt((void *)array);                                     // Detach memory segment
        sem_post(&mutex);                                         // Set semaphore to ready state
        return;                                                   // Process dies
    }
    shmdt((void *)array); // Detach memory segment
    sem_post(&mutex);     // Set semaphore to ready state

    sleep(11); // Sleep amount would be defined by a random from the process creator

    sem_wait(&mutex);                                          // Wait for ready signal
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get id of memory space
    array = (int *)shmat(shmid, 0, 0);                         // Map shared memory to an array
    printf("-----Liberando espacio %d-----\n", counterLocal);  // PRINT FOR TESTING PURPOSES
    for (int pos = 0; pos < SIZE; pos++)                       // Set memory spaces filled by process to -1
    {
        if (array[pos] == counterLocal)
        {
            array[pos] = -1;
        }
    }
    printf("Arreglo actual %d\n", counterLocal); // PRINT FOR TESTING PURPOSES
    for (int pos = 0; pos < SIZE; pos++)         // PRINT FOR TESTING PURPOSES
    {
        printf("%d ", array[pos]);
    }
    printf("\n"); // PRINT FOR TESTING PURPOSES

    // NOT IMPLEMENTED YET
    // Write to log relevant information
    printf("-------Libero espacio %d------\n", counterLocal); // PRINT FOR TESTING PURPOSES
    shmdt((void *)array);                                     // Detach memory segment
    sem_post(&mutex);                                         // Set semaphore to ready state
}

void pageProcess(void *arg)
{
    int counterLocal = counterGlobal; // Save current PID counter locally
    counterGlobal++;                  // Increase global PID counter
    int shmid;
    int full = 0; // Flag for memory availability ; 0 = space was not found, 1 = space was found

    /*
     * Here is where the sync algorithm would be implemented
     * Before waiting at the semaphore, each process would check if it's their turn
     */

    sem_wait(&mutex);                                          // Send wait signal to semaphore when ready
    printf("-------Busca espacio %d-------\n", counterLocal);  // PRINT FOR TESTING PURPOSES
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get memory space
    int *array = (int *)shmat(shmid, 0, 0);                    // Map shared memory space to array
    // Loop to check for available memory
    for (int pos = 0; pos < SIZE; pos++) // Here, SIZE would be replaced by the amount of pages
    {
        if (array[pos] == -1) // If memory is available
        {
            array[pos] = counterLocal; // Fill memory
            full = 1;                  // Flip flag
            break;
        }
    }
    printf("Arreglo actual %d\n", counterLocal); // PRINT FOR TESTING PURPOSES
    for (int pos = 0; pos < SIZE; pos++)         // PRINT FOR TESTING PURPOSES
    {
        printf("%d ", array[pos]);
    }
    printf("\n"); // PRINT FOR TESTING PURPOSES

    if (full) // If space was found in memory
    {
        // NOT IMPLEMENTED YET
        // Write to log relevant information
        printf("------Encontro espacio %d-----\n", counterLocal); // PRINT FOR TESTING PURPOSES
    }
    else // If space was not found in memory
    {
        // NOT IMPLEMENTED YET
        // Write to log relevant information
        printf("----No encontro espacio %d----\n", counterLocal); // PRINT FOR TESTING PURPOSES
        shmdt((void *)array);                                     // Detach memory space
        sem_post(&mutex);                                         // Set semaphore to ready state
        return;                                                   // Process dies
    }

    shmdt((void *)array); // Detach memory space
    sem_post(&mutex);     // Set semaphore to ready state

    sleep(11); // Sleep amount would be defined by a random from the process creator

    sem_wait(&mutex);                                          // Wait for ready signal
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get memory space
    array = (int *)shmat(shmid, 0, 0);                         // Map shared memory space to array
    printf("-----Liberando espacio %d-----\n", counterLocal);  // PRINT FOR TESTING PURPOSES
    for (int pos = 0; pos < SIZE; pos++)                       // Set memory occupied by process back to -1
    {
        if (array[pos] == counterLocal)
        {
            array[pos] = -1;
            break;
        }
    }
    printf("Arreglo actual %d\n", counterLocal); // PRINT FOR TESTING PURPOSES
    for (int pos = 0; pos < SIZE; pos++)
    {
        printf("%d ", array[pos]);
    }
    printf("\n"); // PRINT FOR TESTING PURPOSES

    // NOT IMPLEMENTED YET
    // Write relevant information to log
    printf("-------Libero espacio %d------\n", counterLocal); // PRINT FOR TESTING PURPOSES

    shmdt((void *)array); // Detach memory space
    sem_post(&mutex);     // Set semaphore to ready state
}

int main()
{
    // INIT FUNCTION
    int shmid;
    printf("Enter shared memory size: ");
    scanf("%d", &SIZE); // Ask for shared memory size from user
    printf("\n\n");
    sem_init(&mutex, 0, 1);                                    // initilalize semaphore
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Create shared memory space
    int *array = (int *)shmat(shmid, 0, 0);                    // Map memory space to array
    for (int pos = 0; pos < SIZE; pos++)                       // Initialize empty memory space with -1
    {
        array[pos] = -1;
    }
    // END OF INIT FUNCTION

    // TESTING AREA
    // This part would be done by the process creator
    // segmentProcess or pageProcess would depend on user input
    segSize = 2; // Defined by random in process creator
    pthread_t t1;
    while (counterGlobal != 10)
    {
        pthread_t t2;
        pthread_create(&t2, NULL, segmentProcess, NULL);
        sleep(2); // Fixed sleep amount, defined by random in process creator
    }

    pthread_create(&t1, NULL, segmentProcess, NULL);
    pthread_join(t1, NULL);
    // END OF TESTING AREA

    // Liberate semaphore memory, this would be done by the process finalizer
    sem_destroy(&mutex);
    // Liberate shared memory space, this would be done by the process finalizer
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}