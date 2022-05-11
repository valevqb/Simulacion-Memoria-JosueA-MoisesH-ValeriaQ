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


// SOURCES:
// https://www.geeksforgeeks.org/use-posix-semaphores-c/
// https://stackoverflow.com/questions/40181096/c-linux-pthreads-sending-data-from-one-thread-to-another-using-shared-memory-gi

#define SIZE 5
key_t key = 12345678;
sem_t mutex;


int counterGlobal= 0;

void *threadWrite(void *arg)
{
    int shmid;
    int *array;
    int i = 0;

    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0600);

    array = (int *)shmat(shmid, 0, 0);
    printf("1: waits for signal\n");
    // wait
    sem_wait(&mutex);
    // critical section
    printf("\n1: is Writing..\n");
    sleep(1);
    for (i; i < SIZE; i++)
    {
        sleep(1);
        array[i] = 1;
        printf("1: Wrote %d at %d\n", array[i], i);
    }

    shmdt((void *)array);

    // signal
    printf("\n1: is done writing...\n");
    sem_post(&mutex);
    sleep(1);

    printf("1: waits for signal again\n");
    // wait
    sem_wait(&mutex);
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0600);
    array = (int *)shmat(shmid, 0, 0);
    printf("\n1: is Reading response..\n");
    sleep(1);

    for (i = 0; i < SIZE; i++)
    {
        sleep(1);
        printf("1: Read %d at %d\n", array[i], i);
    }
    // critical section

    shmdt((void *)array);

    // signal
    printf("\n1: is done reading...\n");
    sem_post(&mutex);
}

void *threadRead(void *arg)
{
    int shmid;
    int *array;
    int i = 0;

    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0600);

    array = (int *)shmat(shmid, 0, 0);
    printf("2: waits for signal\n");
    // wait
    sem_wait(&mutex);
    printf("\n2: is reading array..\n");
    sleep(1);
    // critical section
    for (i; i < SIZE; i++)
    {
        sleep(1);
        //  Read
        printf("2: Read %d at %d\n", array[i], i);
    }
    printf("\n2: is responding...\n");
    sleep(1);
    for (i = 0; i < SIZE; i++)
    {
        // Respond
        sleep(1);
        array[i] += 5;
        printf("2: Wrote %d at %d\n", array[i], i);
    }

    shmdt((void *)array);

    // signal
    printf("\n2: is done responding...\n");
    sem_post(&mutex);
}


void Process()
{
    int counterLocal = counterGlobal;
    counterGlobal++;
    int shmid;
    int full = 0;//Flag to find space

	sem_wait(&mutex);
    shmid = shmget(key, SIZE *sizeof(int), IPC_CREAT | 0666);
    int * array = (int*) shmat(shmid, 0, 0);
    for (int pos = 0; pos < SIZE; pos++)
    {
        if( array[pos] == -1)
        {
            array[pos] = counterLocal;
            full = 1;
            break;
        }
    }
    if(full)
    {
       printf("Encontro espacio");//Se escribe en bitacora
    }
    else
    {
        printf("No encontro espacio");//Se escribe en bitacora
        shmdt((void*)array);
        sem_post(&mutex);
        return;    
    }
    shmdt((void*)array);
    sem_post(&mutex);

    sleep(5);
    
    sem_wait(&mutex);
    shmid = shmget(key, SIZE *sizeof(int), IPC_CREAT | 0666);
    array = (int*) shmat(shmid, 0, 0);
    for (int pos = 0; pos < SIZE; pos++)
    {
        if( array[pos] == counterLocal)
        {
            array[pos] = -1;
            full = 1;
            break;
        }
    }
    printf("Libero espacio"); //En bitacora
    shmdt((void*)array);
    sem_post(&mutex);
}

int main()
{

    int shmid;

    sem_init(&mutex, 0, 1);
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666);
    int * array = (int*) shmat(shmid, 0, 0);
    for (int pos = 0; pos < SIZE; pos++)
    {
        array[pos] = -1;
    }
    
    pthread_t t1, t2;
    pthread_create(&t1, NULL, Process, NULL);
    pthread_create(&t2, NULL, Process, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    sem_destroy(&mutex);

    shmctl(shmid, IPC_RMID, NULL);

	return 0;

}