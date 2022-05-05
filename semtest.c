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

#define SIZE 10
sem_t mutex;

void *threadWrite(void *arg)
{
    int shmid;
    int *array;
    int i = 0;
    key_t key = 12345678;

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
        sleep(2);
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
        sleep(2);
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
    key_t key = 12345678;

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
        sleep(2);
        // Read
        printf("2: Read %d at %d\n", array[i], i);
    }
    printf("\n2: is responding...\n");
    sleep(1);
    for (i = 0; i < SIZE; i++)
    {
        // Respond
        sleep(2);
        array[i] += 5;
        printf("2: Wrote %d at %d\n", array[i], i);
    }

    shmdt((void *)array);

    // signal
    printf("\n2: is done responding...\n");
    sem_post(&mutex);
}

int main()
{
    int i;
    int shm_id;
    pid_t pid;
    int *addr;
    int data;
    pid_t current_pid;
    key_t shm_key;

    sem_init(&mutex, 0, 1);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, threadWrite, NULL);
    pthread_create(&t2, NULL, threadRead, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    sem_destroy(&mutex);
    return 0;
}