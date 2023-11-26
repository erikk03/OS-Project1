#include "header.h"

int main()
{
	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;
	
	//srand((unsigned int)getpid()); //-

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
	shared_stuff->written_by_A = 0;
    
    pthread_t thread1, thread2;
    
    /* Initialize semaphores as process-shared, with value 0. */
    if (sem_init(&shared_stuff->sem1, 1, 0) == -1){
        errExit("sem_init-sem1");
    }
    
    if (sem_init(&shared_stuff->sem_packet1, 1, 0) == -1){
        errExit("sem_init-sem_packet1");
    }

    /*SEND MESSAGE TO A*/
    pthread_create(&thread1, NULL, produce, (void*)shared_stuff);
    
    /*RECIEVE MESSAGE FROM A*/
    pthread_create(&thread2, NULL, consume, (void*)shared_stuff);
    
    pthread_join( thread2, NULL);
    if(shared_stuff->cancelation == 0){
        pthread_cancel(thread1);
    }
    else
        pthread_join( thread1, NULL);

    // Detach shared memory segment
	if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}

	if (shmctl(shmid, IPC_RMID, 0) == -1) {
		fprintf(stderr, "shmctl(IPC_RMID) failed\n");
		exit(EXIT_FAILURE);
	}


	exit(EXIT_SUCCESS);
}

void *consume(void *shared_s){
    struct shared_use_st *shared_stuff = shared_s;
    char local_buffer[TEXT_SZ];
    
    while(shared_stuff->running){
        
        /* Wait for 'sem1' to be posted by peer before touching shared memory. */
        if (sem_wait(&shared_stuff->sem1) == -1){
            errExit("sem_wait");
        }
        
        strcat(local_buffer, shared_stuff->text_packetA);
        
        if (sem_post(&shared_stuff->sem_packet1) == -1){
            errExit("sem_post");
        }

        if(shared_stuff->last_packet == 1){
            printf("\nA wrote: %s", local_buffer);
            memset(local_buffer, '\0', TEXT_SZ);
        }
        
        // shared_stuff->written_by_A = 0;

        if (strncmp(shared_stuff->some_textA, "BYE", 3) == 0){
            shared_stuff->running = 0;
        }
        
    }
    pthread_exit("temp_text");
}

void *produce(void *shared_s){
    struct shared_use_st *shared_stuff = shared_s;

    while(shared_stuff->running) {
        printf("Enter some text: ");
        fgets(shared_stuff->some_textB, TEXT_SZ, stdin);
        //shared_stuff->written_by_B = 1;
        shared_stuff->last_packet = 0;

        for(int i = 0; i < strlen(shared_stuff->some_textB); i=i+15){
            memset(shared_stuff->text_packetB, 0, 15);   
            strncpy(shared_stuff->text_packetB, shared_stuff->some_textB + i, 15);
            
            if(i >= strlen(shared_stuff->some_textA) - 15){
                shared_stuff->last_packet = 1;
            }

            /* Post 'sem2' to tell the peer that it can now access the modified data in shared memory. */
            if (sem_post(&shared_stuff->sem2) == -1){
                errExit("sem_post");
            }
            if (sem_wait(&shared_stuff->sem_packet2) == -1){
                errExit("sem_wait");
            }        
        }

        if (strncmp(shared_stuff->some_textB, "BYE", 3) == 0) {
            shared_stuff->running = 0;
            shared_stuff->cancelation = 1;
            if (sem_post(&shared_stuff->sem1) == -1){
                errExit("sem_post");
            }
        }

    }

    pthread_exit("temp_text");
}