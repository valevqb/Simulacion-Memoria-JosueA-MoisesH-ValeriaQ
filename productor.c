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
key_t keyStruct = 1122233;
key_t keyStructSize = 44332211;

sem_t mutex; //semaphore of memory functions
sem_t espia; //semaphore of espia program functions

int counterGlobal = 0; // Counter for PID of processes, would be managed by process structure
int SIZE; // Size of shared memory, given by user input in init function
int *array;
struct Queue *cola;
int tamanio;

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
	int spaces; // Segments or pages space
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
	int size;
};

//Creates spia program array
void createArray()
{
	while (1)
	{
		if (tamanio == cola->size)
		{
			sem_wait(&espia);
			tamanio = cola->size;
			struct Node *tmp = cola->first;
			for (int i = 0; i < cola->size; i++)
			{
				array[i] = tmp->process.state;
				tmp = tmp->next;
			}
			sem_post(&espia);
		}
	}
}

//Creates espia shared memory
void createSharedMemoryEspia()
{
	int shmstructsize;
	int *mapStructSize;
	int shmarray;
	tamanio = -1;

	while (1)
	{
		sem_wait(&espia);
		if (tamanio != cola->size)
		{
			tamanio = cola->size;
			shmctl(shmarray, IPC_RMID, NULL);										  //
			shmarray = shmget(keyStruct, cola->size * sizeof(int), IPC_CREAT | 0666); // Create shared memory space
			array = (int *)shmat(shmarray, 0, 0);									  // Map memory space to array

			shmstructsize = shmget(keyStructSize, sizeof(int), IPC_CREAT | 0666); // Create shared memory space by size
			mapStructSize = (int *)shmat(shmstructsize, 0, 0);					  // Map shared memory space to array
			mapStructSize[0] = cola->size;
		}
		sem_post(&espia);
	}
}

// Inserts process into the queue
int insertProcess(struct Queue *q, struct PCB pcb)
{
	sem_wait(&espia);
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
	q->size += 1;
	sem_post(&espia);
	return 0;
}

//Search process by Id
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

//Write bitacora file
void writeBit(int pId, int state, int *array, int spaces[], int sizes)
{
	FILE *file = fopen("bitacora.txt", "a"); // Open file
	
	//Gets time
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	fprintf(file,
			"Actual Time: %d-%02d-%02d %02d:%02d:%02d\n",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	
	if (state == 1)
	{
		printf("Process %d with size %d is searching for memory space.\n", pId, sizes);		 
		fprintf(file, "Process %d with size %d is searching for memory space.\n", pId, sizes); // Write in file
	}
	else if (state == 2)
	{
		printf("Process %d found space in memory.\n", pId);	// PRINT FOR TESTING PURPOSES
		fprintf(file, "Process %d was inserted into the next memory location(s):\n", pId); // Write in file
		for (int i = 0; i < sizes; i++){
			fprintf(file, "%d, ", spaces[i]);
		}
		fprintf(file, "\n", pId); // Write in file
	}
	else if (state == 3)
	{
		printf("Process %d died, it did not found memory space.\n", pId);
		fprintf(file, "Process %d died, it did not found memory space.\n", pId);			   // Write in file
	}
	else if (state == 4)
	{
		printf("Process %d finished, its space is free.\n", pId);
		fprintf(file, "Process %d finished, its space  is free.\n", pId); // Write in file
	}

	fprintf(file, "Actual array (-1 means free space): \n");

	for (int pos = 0; pos < SIZE; pos++) // Write in file
	{
		fprintf(file, "%d ", array[pos]);
	}
	fprintf(file, "\n\n");
	
	fclose(file);
}

//If paging was select use it
void pageProcess(struct Node *arg)
{
	int idProcess = arg->process.pId; // Save current PID counter locally
	int shmid;
	int full = 0;	  // Flag for memory availability ; 0 = space was not found, 1 = space was found
	int segCount = 0; // Counter to check if there is enough continuous space

	/*
	 * Here is where the sync algorithm would be implemented
	 * Before waiting at the semaphore, each process would check if it's their turn
	 */
	sem_wait(&mutex);						 // Send wait signal to semaphore when ready
	arg->process.state = 1;					 // Search memory

	shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get shared memory
	int *array = (int *)shmat(shmid, 0, 0);					   // Map memory to array

	writeBit(idProcess, 1, array, NULL, arg->process.spaces);
	int location[arg->process.spaces];
	int countSpace = 0;

	// Loop to check for available memory
	for (int pos = 0; pos < SIZE; pos++) // Here, SIZE would be replaced by the amount of segments
	{
		if (array[pos] == -1) // If the memory space is available
		{
			segCount++; // Increase segment count
			array[pos] = idProcess;
			location[countSpace] = pos;
			countSpace++;
		}
		if (segCount == arg->process.spaces) // If all the space was found
		{
			full = 1; // Flip flag
			break;	  // stop searching
		}
	}

	if (full) // If there was memory available
	{
		arg->process.state = 2; // Process in memory
		writeBit(idProcess, 2, array, location, countSpace);
	}
	else // If there was no memory available
	{
		for (int pos = 0; pos < SIZE; pos++) // Set memory occupied by process back to -1
		{
			if (array[pos] == idProcess)
			{
				array[pos] = -1;
			}
		}
		arg->process.state = 3; // Process dies
		writeBit(idProcess, 3, array, NULL, arg->process.spaces);
		shmdt((void *)array); // Detach memory segment
		sem_post(&mutex);	  // Set semaphore to ready state
		return; // Process dies
	}

	shmdt((void *)array); // Detach memory segment
	sem_post(&mutex); // Set semaphore to ready state

	srand(time(NULL));
	int pageSleep = rand() % (60 - 20 + 1) + 20;
	sleep(pageSleep); // Sleep amount would be defined by a random from the process creator

	sem_wait(&mutex);										   // Wait for ready signal
	shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get id of memory space
	array = (int *)shmat(shmid, 0, 0);						   // Map shared memory to an array

	for (int pos = 0; pos < SIZE; pos++) // Set memory spaces filled by process to -1
	{
		if (array[pos] == idProcess)
		{
			array[pos] = -1;
		}
	}
	
	arg->process.state = 4; // Process finish
	writeBit(idProcess, 4, array, NULL, arg->process.spaces);
	sem_post(&mutex); // Set semaphore to ready state
}

//If segment was selected use it
void segmentProcess(struct Node *arg)
{
	int idProcess = arg->process.pId; // Save current PID counter locally
	int shmid;
	int full = 0; // Flag for memory availability ; 0 = space was not found, 1 = space was found

	/*
	 * Here is where the sync algorithm would be implemented
	 * Before waiting at the semaphore, each process would check if it's their turn
	 */

	sem_wait(&mutex);						 // Send wait signal to semaphore when ready
	arg->process.state = 1;					 // Search memory

	shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get memory space
	int *array = (int *)shmat(shmid, 0, 0);					   // Map shared memory space to array
	int totalSp = 0;
	for (int i = 0; arg->process.spaces > i; i++){
		totalSp = totalSp + arg->process.sizeP[i];
	}
	int location[totalSp];
	int countSpace = 0;
	writeBit(idProcess, 1, array, NULL, totalSp);

	// Loop to check for available memory
	for (int pos = 0; pos < arg->process.spaces; pos++) // Here, SIZE would be replaced by the amount of pages
	{
		int segCount = 0;
		full = 0;

		for (int pos1 = 0; pos1 < SIZE; pos1++) // Here, SIZE would be replaced by the amount of segments
		{
			if (array[pos1] == -1) // If the memory space is available
			{
				segCount++;								 // Increase segment count
				if (segCount == arg->process.sizeP[pos]) // If the segment count is equal to the required space
				{
					while (segCount != 0) // Fill the available space
					{
						segCount--;
						array[pos1 - segCount] = idProcess;
						location[countSpace] = pos1 - segCount;
						countSpace++;
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

		if (full == 0)
		{ // Space not found
			break;
		}
	}

	if (full) // If space was found in memory
	{
		arg->process.state = 2; // Process in memory
		writeBit(idProcess, 2, array, location, countSpace);
	}
	else // If space was not found in memory
	{
		for (int pos = 0; pos < SIZE; pos++) // Set memory occupied by process back to -1
		{
			if (array[pos] == idProcess)
			{
				array[pos] = -1;
			}
		}
		arg->process.state = 3; // Process dies
		writeBit(idProcess, 3, array, NULL, totalSp);
		sem_post(&mutex);	  // Set semaphore to ready state
		return;				  // Process dies
	}

	sem_post(&mutex);	  // Set semaphore to ready state

	srand(time(NULL));
	int segmentSleep = rand() % (60 - 20 + 1) + 20;
	sleep(segmentSleep); // Sleep amount would be defined by a random from the process creator

	sem_wait(&mutex);										   // Wait for ready signal
	shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666); // Get memory space
	array = (int *)shmat(shmid, 0, 0);						   // Map shared memory space to array
	for (int pos = 0; pos < SIZE; pos++)					   // Set memory occupied by process back to -1
	{
		if (array[pos] == idProcess)
		{
			array[pos] = -1;
		}
	}
	
	arg->process.state = 4; // Process finish
	writeBit(idProcess, 4, array, NULL, totalSp);
	sem_post(&mutex);	  // Set semaphore to ready state
}

//main menu
int menu(){
	int type;
	
	printf("\nSelect a memory management scheme\n");
	printf("1. Paging\n");
	printf("2. Segmentation\n");
	scanf("%d", &type); // Ask for shared memory size from user
	if(type == 1){
		printf("\nYou have selected paging.\n");
		printf("-------------------------------\n\n");
		return 1;
	}
	else if(type == 2){
		printf("\nYou have selected segmentation.\n");
		printf("-------------------------------\n\n");
		return 2;
	} else{
		printf("\nPlease select 1 or 2.\n\n");
		menu();
	}
}

int main()
{
	// INIT FUNCTION
	int shmid;
	int shmsize;
	int *mapSize;

	// Select variables
	int type;

	shmsize = shmget(keySize, sizeof(int), IPC_CREAT | 0666); // Get shared memory size

	mapSize = (int *)shmat(shmsize, 0, 0);
	SIZE = mapSize[0];
	
	if(SIZE == 0){
		printf("\nPlease create a shared memory with a size greater than 0.\n\n");
		return 0;
	}

	//for saving data
	cola = (struct Queue *)malloc(sizeof(struct Queue));
	cola->size = 0;

	type = menu();

	// INIT FUNCTION
	sem_init(&mutex, 0, 1); // initilalize semaphore
	sem_init(&espia, 0, 1); // initilalize semaphore

	// This part would be done by the process creator
	// segmentProcess or pageProcess would depend on user input
	// validate the program still runing
	pthread_t t1;
	pthread_t t3;
	pthread_t t4;

	pthread_create(&t4, NULL, createSharedMemoryEspia, NULL);
	pthread_create(&t3, NULL, createArray, NULL);

	srand(time(NULL));
	int pthreadTime;
	int segments;
	
	while (SIZE > 0)
	{
		shmsize = shmget(keySize, sizeof(int), IPC_CREAT | 0666); // Get shared memory size
		mapSize = (int *)shmat(shmsize, 0, 0);
		if(mapSize[0] == NULL){
			printf("\nMemory space was ended.\n\n");
			break;
		}
		
		struct PCB process;
		process.pId = counterGlobal;
		process.state = 0;
		insertProcess(cola, process);

		if (type == 2) //If segmentation was selected
		{
			pthread_t t2;

			segments = rand() % (5 - 1 + 1) + 1;
			for (int i = 0; segments > i; i++)
			{
				cola->last->process.sizeP[i] = rand() % (3 - 1 + 1) + 1;
				printf("Segment #%d, with size of %d memory line(s)\n", i, cola->last->process.sizeP[i]);
			}
			cola->last->process.spaces = segments;
			pthread_create(&t2, NULL, segmentProcess, cola->last);
		} 
		else //If paging was selected
		{
			cola->last->process.spaces = rand() % (10 - 1 + 1) + 1;
			pthread_t t2;
			pthread_create(&t2, NULL, pageProcess, cola->last);
		}
		pthreadTime = rand() % (60 - 30 + 1) + 30;
		sleep(pthreadTime); // Fixed sleep amount, defined by random in process creator
		counterGlobal++;
	}

	// Liberates semaphore memory, this would be done by the process finalizer
	sem_destroy(&mutex);
	sem_destroy(&espia);

	return 0;
}

