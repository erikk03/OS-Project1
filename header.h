#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/shm.h>
#include <semaphore.h>

#define TEXT_SZ 2048
#define BUFF_SIZE 15

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)


////////////////        Structs     /////////////////

struct shared_use_st {
    int running;                // Tells if a process is running

    sem_t sem1;                 // POSIX unnamed semaphore
    sem_t sem2;                 // POSIX unnamed semaphore

    sem_t sem_packet1;
    sem_t sem_packet2;

	int written_by_A;           // Defines which process is writing something
    int written_by_B;

	char some_textA[TEXT_SZ];   // Buffer for each process
    char some_textB[TEXT_SZ];

    char text_packetA[BUFF_SIZE];   // Buffer for message transfer of 15 bytes
    char text_packetB[BUFF_SIZE];

    int cancelation;            // Tells which process terminated the program. 0 for mainA, 1 for mainB
};

///////////////   Thread functions    /////////////////

// Produces a message and sends it to the other process
void *produce(void *shared_s);

// Gets a message from the other process and prints it
void *consume(void *shared_s);