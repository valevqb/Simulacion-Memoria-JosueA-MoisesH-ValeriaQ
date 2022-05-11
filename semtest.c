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

int counterGlobal = 0;

void Process(void *arg)
{
    int counterLocal = counterGlobal;
    counterGlobal++;
    int shmid;
    int full = 0; // Flag to find space

    sem_wait(&mutex);
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666);
    int *array = (int *)shmat(shmid, 0, 0);
    printf("Entro a arreglo %d\n", counterLocal);
    for (int pos = 0; pos < SIZE; pos++)
    {
        if (array[pos] == -1)
        {
            array[pos] = counterLocal;
            full = 1;
            break;
        }
    }
    printf("Arreglo actual %d\n", counterLocal);
    for (int pos = 0; pos < SIZE; pos++)
    {
        printf("%d ", array[pos]);
    }
    printf("\n");

    if (full)
    {
        printf("Encontro espacio %d\n", counterLocal); // Se escribe en bitacora
    }
    else
    {
        printf("No encontro espacio %d\n", counterLocal); // Se escribe en bitacora
        shmdt((void *)array);
        sem_post(&mutex);
        return;
    }
    shmdt((void *)array);
    sem_post(&mutex);

    sleep(6);

    sem_wait(&mutex);
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666);
    array = (int *)shmat(shmid, 0, 0);
    printf("Liberando espacio %d\n", counterLocal);
    for (int pos = 0; pos < SIZE; pos++)
    {
        if (array[pos] == counterLocal)
        {
            array[pos] = -1;
            full = 1;
            break;
        }
    }
    printf("Arreglo actual %d\n", counterLocal);
    for (int pos = 0; pos < SIZE; pos++)
    {
        printf("%d ", array[pos]);
    }
    printf("\n");
    printf("Libero espacio %d\n", counterLocal); // En bitacora
    shmdt((void *)array);
    sem_post(&mutex);
}

int main()
{

    int shmid;

    sem_init(&mutex, 0, 1);
    shmid = shmget(key, SIZE * sizeof(int), IPC_CREAT | 0666);
    int *array = (int *)shmat(shmid, 0, 0);
    for (int pos = 0; pos < SIZE; pos++)
    {
        array[pos] = -1;
    }
    pthread_t t1;
    while (counterGlobal != 20)
    {
        pthread_t t2;
        pthread_create(&t2, NULL, Process, NULL);
        sleep(1);
    }

    pthread_create(&t1, NULL, Process, NULL);
    pthread_join(t1, NULL);
    sem_destroy(&mutex);

    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}