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
key_t keySize = 987;
sem_t mutex;

int counterGlobal = 0; // Counter for PID of processes, would be managed by process structure
//int sizeProcessPage;           // This would be generated randomly by the init function
//int sizeProcessSeg; 
int SIZE ;              // Size of shared memory, given by user input in init function

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
	int sizeP[5];
	int spaces;	//Segments or pages space
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

void printNode(struct Node *n)
{
    printf("\nProcess ID:%d\nSpaces:%d\nState:%d\n", n->process.pId, n->process.spaces, n->process.state);
    return;
}

// Prints lists
void printQueue(struct Queue *q)
{
    struct Node *tmp = q->first;
    while (tmp != NULL)
    {
        printNode(tmp);
        tmp = tmp->next;
    }

    return;
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

void pageProcess(struct Node *arg)
{
    int idProcess = arg->process.pId; // Save current PID counter locally
    int shmid;
    int full = 0;     				  // Flag for memory availability ; 0 = space was not found, 1 = space was found
    int segCount = 0;                 // Counter to check if there is enough continuous space

    /*
     * Here is where the sync algorithm would be implemented
     * Before waiting at the semaphore, each process would check if it's their turn
     */
    sem_wait(&mutex);                                         				// Send wait signal to semaphore when ready
	FILE *file = fopen("bitacora.txt","a");                                 // Open file
    arg->process.state = 1;												    //Search memory
	
	shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get shared memory
    int *array = (int *)shmat(shmid, 0, 0);                    // Map memory to array
	
	writeBit(idProcess, 1, array, file);
	
    // Loop to check for available memory
    for (int pos = 0; pos < SIZE; pos++) // Here, SIZE would be replaced by the amount of segments
    {
        if (array[pos] == -1) // If the memory space is available
        {
            segCount++;              // Increase segment count
            if (segCount == arg->process.spaces) // If the segment count is equal to the required space
            {
                while (segCount != 0) // Fill the available space
                {
                    segCount--;
                    array[pos - segCount] = idProcess;
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
		arg->process.state = 2; //Process in memory
		writeBit(idProcess, 2, array, file);
    }
    else // If there was no memory available
    {
		arg->process.state = 3;                                   //Process dies
		writeBit(idProcess, 3, array, file);
        shmdt((void *)array);                                     // Detach memory segment
        sem_post(&mutex);                                         // Set semaphore to ready state
		fclose(file);
        return;                                                   // Process dies
    }
	
    shmdt((void *)array); // Detach memory segment
	fclose(file);
    sem_post(&mutex);     // Set semaphore to ready state

	srand(time(NULL));
	int pageSleep = rand () % (60-20+1) + 20;
    sleep(pageSleep); // Sleep amount would be defined by a random from the process creator

    sem_wait(&mutex);                                          // Wait for ready signal
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get id of memory space
    array = (int *)shmat(shmid, 0, 0);                         // Map shared memory to an array
	
    for (int pos = 0; pos < SIZE; pos++)                       // Set memory spaces filled by process to -1
    {
        if (array[pos] == idProcess)
        {
            array[pos] = -1;
        }
    }
	file = fopen("bitacora.txt","a");
    arg->process.state = 4;                                   //Process finish
	writeBit(idProcess, 4, array, file);
    shmdt((void *)array);                                     // Detach memory segment
	fclose(file);
    sem_post(&mutex);                                         // Set semaphore to ready state
}

void segmentProcess(struct Node *arg)
{
	int idProcess = arg->process.pId; // Save current PID counter locally
    int shmid;
    int full = 0; // Flag for memory availability ; 0 = space was not found, 1 = space was found

    /*
     * Here is where the sync algorithm would be implemented
     * Before waiting at the semaphore, each process would check if it's their turn
     */
	
	for (int i = 0; i < arg->process.spaces; i++){
		printf("Espacios %d",arg->process.sizeP[i]);
	} printf("\n\n");

    sem_wait(&mutex);                                          // Send wait signal to semaphore when ready
	FILE *file = fopen("bitacora.txt","a");                                 // Open file
    arg->process.state = 1;												    //Search memory
	
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get memory space
    int *array = (int *)shmat(shmid, 0, 0);                    // Map shared memory space to array
	writeBit(idProcess, 1, array, file);
	
    // Loop to check for available memory
    for (int pos = 0; pos < arg->process.spaces; pos++) // Here, SIZE would be replaced by the amount of pages
    {
		int segCount = 0;
		full = 0;
		
		for (int pos1 = 0; pos1 < SIZE; pos1++) // Here, SIZE would be replaced by the amount of segments
		{
			if (array[pos1] == -1) // If the memory space is available
			{
				segCount++;              // Increase segment count
				if (segCount == arg->process.sizeP[pos]) // If the segment count is equal to the required space
				{
					while (segCount != 0) // Fill the available space
					{
						segCount--;
						array[pos1 - segCount] = idProcess;
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
		
		if(full == 0){ //Space not found
			break;
		}
    }

    if (full) // If space was found in memory
    {
        arg->process.state = 2; //Process in memory
		writeBit(idProcess, 2, array, file);
    }
    else // If space was not found in memory
    {
		for (int pos = 0; pos < SIZE; pos++)                       // Set memory occupied by process back to -1
		{
			if (array[pos] == idProcess)
			{
				array[pos] = -1;
			}
		}
        arg->process.state = 3;                                   //Process dies
		writeBit(idProcess, 3, array, file);
		fclose(file);
        shmdt((void *)array);                                     // Detach memory space
        sem_post(&mutex);                                         // Set semaphore to ready state
        return;                                                   // Process dies
    }
	
	fclose(file);

    shmdt((void *)array); // Detach memory space
    sem_post(&mutex);     // Set semaphore to ready state

    srand(time(NULL));
	int segmentSleep = rand () % (60-20+1) + 20;
    sleep(segmentSleep); // Sleep amount would be defined by a random from the process creator

    sem_wait(&mutex);                                          // Wait for ready signal
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get memory space
    array = (int *)shmat(shmid, 0, 0);                         // Map shared memory space to array
    for (int pos = 0; pos < SIZE; pos++)                       // Set memory occupied by process back to -1
    {
        if (array[pos] == idProcess)
        {
            array[pos] = -1;
        }
    }
	file = fopen("bitacora.txt","a");
    arg->process.state = 4;                                   //Process finish
	writeBit(idProcess, 4, array, file);
	fclose(file);
    shmdt((void *)array); // Detach memory space
    sem_post(&mutex);     // Set semaphore to ready state
}

int main()
{

	// INIT FUNCTION
    int shmid;
	int shmsize;
	int* mapSize;
	//Select variables
	int type;

	shmsize = shmget(keySize, sizeof(int), IPC_CREAT | 0666); // Get shared memory size

	mapSize = (int*)shmat(shmsize, 0, 0);
	SIZE = mapSize[0];


	shmdt((void *)mapSize); // Detach memory space
	printf("Size: %d \n", SIZE);


	struct Queue *cola = (struct Queue *)malloc(sizeof(struct Queue));
	
	
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
    pthread_t t1;
	srand(time(NULL));
	int pthreadTime;
	int segments;
    while (1)
    {	
		struct PCB process;
		pthreadTime = rand () % (60-30+1) + 30;
		process.pId = counterGlobal;
		process.state = 0;
		insertProcess(cola, process);

		if(type == 2){
			pthread_t t2;
			
			segments = rand () % (5-1+1) + 1;
			for(int i = 0; segments > i; i++){
				cola->last->process.sizeP[i] = rand () % (3-1+1) + 1;
				printf("Espacios %d", cola->last->process.sizeP[i]);
			}
			printf("\n");
			
			cola->last->process.spaces = segments;
			pthread_create(&t2, NULL, segmentProcess, cola->last);
			
		} else{
			cola->last->process.spaces = rand () % (10-1+1) + 1;
			pthread_t t2;
			pthread_create(&t2, NULL, pageProcess, cola->last);
		}
		printQueue(cola);
		printf("\n\n");
		
		sleep(pthreadTime); // Fixed sleep amount, defined by random in process creator
		
		counterGlobal++;
		
    }

    pthread_create(&t1, NULL, segmentProcess, NULL);
    pthread_join(t1, NULL);
    // END OF TESTING AREA

    // Liberate semaphore memory, this would be done by the process finalizer
    sem_destroy(&mutex);

    return 0;
}

