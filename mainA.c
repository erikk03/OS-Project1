#include "header.h"

int main()
{
	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;

	int shmid;
	shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666 | IPC_CREAT);
	if (shmid == -1) {
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}

	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1) {
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	printf("Shared memory segment with id %d attached at %p\n", shmid, shared_memory);


	shared_stuff = (struct shared_use_st *)shared_memory;
    shared_stuff->written_by_B = 0;

    pthread_t thread1, thread2;

    /* Initialize semaphores as process-shared, with value 0. */
    if (sem_init(&shared_stuff->sem1, 1, 0) == -1)
        errExit("sem_init-sem1");

    /*SEND MESSAGE TO B*/
    pthread_create(&thread1, NULL, produce, (void*)shared_stuff);
    

    /*RECIEVE MESSAGE FROM B*/
    pthread_create(&thread2, NULL, consume, (void*)shared_stuff);
    
    pthread_join( thread1, NULL);
	pthread_join( thread2, NULL);
    
    /* Post 'sem2' to tell the peer that it can now
        access the modified data in shared memory. */

    if (sem_post(&shared_stuff->sem2) == -1)
        errExit("sem_post");

    // Detach shared memory segment
	if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

void *consume(void *shared_s){
    struct shared_use_st *shared_stuff = shared_s;
    int running = 1;

    /* Wait for 'sem1' to be posted by peer before touching shared memory. */
    if (sem_wait(&shared_stuff->sem1) == -1)
        errExit("sem_wait");

    while(running){
        if (shared_stuff->written_by_B){
            printf("B wrote: %s", shared_stuff->some_textB);
            sleep( rand() % 4 );
            shared_stuff->written_by_B = 0;

            if (strncmp(shared_stuff->some_textB, "BYE", 3) == 0){
                running = 0;
            }
        }
    }
    pthread_exit("temp_text");
}

void *produce(void *shared_s){
    struct shared_use_st *shared_stuff = shared_s;
    char buffer[BUFSIZ];
    int running = 1;

    while(running) {
        while(shared_stuff->written_by_A == 1) {
            sleep(2);//-
            printf("waiting for user...\n");
        }
        printf("Enter some text: ");
        fgets(buffer, BUFSIZ, stdin);
        strncpy(shared_stuff->some_textA, buffer, TEXT_SZ);
        shared_stuff->written_by_A = 1;

        if (strncmp(buffer, "BYE", 3) == 0) {
            printf("FINISH");
            running = 0;
        }
    }

    /* Post 'sem2' to tell the peer that it can now access the modified data in shared memory. */
    if (sem_post(&shared_stuff->sem2) == -1)
        errExit("sem_post");

    pthread_exit("temp_text");
}