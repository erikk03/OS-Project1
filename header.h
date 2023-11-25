#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/shm.h>
#include <semaphore.h>

#define TEXT_SZ 2048

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)


struct shared_use_st {
    sem_t  sem1;            /* POSIX unnamed semaphore */
    sem_t  sem2;            /* POSIX unnamed semaphore */

	int written_by_A;
    int written_by_B;

	char some_textA[TEXT_SZ];
    char some_textB[TEXT_SZ];
};

// Thread functions
void *produce(void *shared_s);
void *consume(void *shared_s);