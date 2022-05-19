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
#include <time.h>

key_t key = 12345678;
key_t structKey = 11223344;
sem_t mutex;

int counterGlobal = 0; // Counter for PID of processes, would be managed by process structure
int segSize;           // This would be generated randomly by the init function
int SIZE = 9;              // Size of shared memory, given by user input in init function

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

void writeBit(int pId, int state, int *array, FILE *openFile){
	time_t t = time(NULL);
 	struct tm tm = *localtime(&t);
	fprintf(openFile,
			"Actual Time: %d-%02d-%02d %02d:%02d:%02d\n",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	
	if (state == 1){
		printf("-------Busca espacio %d-------\n", pId);  	        // PRINT FOR TESTING PURPOSES
		fprintf(openFile, "Search in memory with id: %d\n", pId);	//Write in file
	} else if (state == 2){
		printf("------Encontro espacio %d-----\n", pId);	// PRINT FOR TESTING PURPOSES
		fprintf(openFile, "Space found for id: %d\n", pId);	    //Write in file
	} else if (state == 3){
		printf("----No encontro espacio muere proceso %d----\n", pId);     // PRINT FOR TESTING PURPOSES
		fprintf(openFile, "Process dies id: %d\n", pId);	//Write in file
	} else if (state == 4){
		printf("-------Libero espacio %d------\n", pId);         	// PRINT FOR TESTING PURPOSES
		fprintf(openFile, "Process finish with id: %d\n", pId);		//Write in file
	}
	
	fprintf(openFile, "Actual array: \n"); // PRINT FOR TESTING PURPOSES
	
    for (int pos = 0; pos < SIZE; pos++)         // Print memory space status, FOR TESTING PURPOSES
    {
        fprintf(openFile, "%d ", array[pos]);
    }
    fprintf(openFile, "\n\n"); // PRINT FOR TESTING PURPOSES
}

void segmentProcess(struct Node *arg)
{
    int counterLocal = counterGlobal; // Save current PID counter locally
    int shmid;
    int full = 0;     				  // Flag for memory availability ; 0 = space was not found, 1 = space was found
    int segCount = 0;                 // Counter to check if there is enough continuous space
	FILE *file = fopen("bitacora.txt","a");

    /*
     * Here is where the sync algorithm would be implemented
     * Before waiting at the semaphore, each process would check if it's their turn
     */

    sem_wait(&mutex);                                         				// Send wait signal to semaphore when ready
    arg->process.state = 1;												    //Search memory
	
	shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get shared memory
    int *array = (int *)shmat(shmid, 0, 0);                    // Map memory to array
	
	writeBit(arg->process.pId, 1, array, file);
	
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

    if (full) // If there was memory available
    {
        // NOT IMPLEMENTED
        // Write to log relevant information
		arg->process.state = 2; //Process in memory
		writeBit(arg->process.pId, 2, array, file);
    }
    else // If there was no memory available
    {
        // NOT IMPLEMENTED
        // Write to log relevant information
		arg->process.state = 3;                                   //Process dies
		writeBit(arg->process.pId, 3, array, file);
        shmdt((void *)array);                                     // Detach memory segment
        sem_post(&mutex);                                         // Set semaphore to ready state
		fclose(file);
        return;                                                   // Process dies
    }
	
    shmdt((void *)array); // Detach memory segment
    sem_post(&mutex);     // Set semaphore to ready state

    sleep(11); // Sleep amount would be defined by a random from the process creator

    sem_wait(&mutex);                                          // Wait for ready signal
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get id of memory space
    array = (int *)shmat(shmid, 0, 0);                         // Map shared memory to an array
	
    for (int pos = 0; pos < SIZE; pos++)                       // Set memory spaces filled by process to -1
    {
        if (array[pos] == counterLocal)
        {
            array[pos] = -1;
        }
    }

    arg->process.state = 4;                                   //Process finish
	writeBit(arg->process.pId, 4, array, file);
    shmdt((void *)array);                                     // Detach memory segment
    sem_post(&mutex);                                         // Set semaphore to ready state
	fclose(file);
}

void pageProcess(void *arg)
{
    int counterLocal = counterGlobal; // Save current PID counter locally
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
        // NOT IMPLEMENTED YETpId
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
    struct PCB process ;
	//Select variables
	int type;

    struct Node *tmp = malloc(sizeof(struct Node));
    shmid = shmget(structKey, sizeof(tmp), IPC_CREAT | 0666); // Create shared memory space

    tmp = (struct Node *)shmat(shmid, 0, 0); // Map shared memory space to array
    process.pId = 0;
    process.state = 0;
    tmp->process = process;
	
	printf("Select type of memory process\n");
	printf("1. Page\n");
	printf("2. Segment\n");
    scanf("%d", &type); // Ask for shared memory size from user
    printf("\n\n");   
	
    // INIT FUNCTION
    sem_init(&mutex, 0, 1);	// initilalize semaphore

    // TESTING AREA
    // This part would be done by the process creator
    // segmentProcess or pageProcess would depend on user input
    segSize = 2; // Defined by random in process creator
    pthread_t t1;
    while (1)
    {	
		if(type == 2){
			pthread_t t2;
			pthread_create(&t2, NULL, segmentProcess, tmp);
			sleep(2); // Fixed sleep amount, defined by random in process creator
		} else{
			pthread_t t2;
			pthread_create(&t2, NULL, pageProcess, tmp);
			sleep(2); // Fixed sleep amount, defined by random in process creator
		}
		
		counterGlobal++;
		
		struct Node *tmp2 = malloc(sizeof(struct Node));
		process.pId = counterGlobal;
		process.state = 0;
		tmp2->process = process;
		tmp->next = tmp2;
		tmp = tmp2;
    }

    pthread_create(&t1, NULL, segmentProcess, NULL);
    pthread_join(t1, NULL);
    // END OF TESTING AREA

    // Liberate semaphore memory, this would be done by the process finalizer
    sem_destroy(&mutex);

    return 0;
}

