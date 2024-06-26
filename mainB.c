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

    // Initialize struct
	shared_stuff = (struct shared_use_st *)shared_memory;
    shared_stuff->running = 1;
    shared_stuff->messages_recievedB = 0;
    shared_stuff->messages_sentB = 0;
    shared_stuff->total_packages_sentB = 0;
    shared_stuff->total_packages_recievedB = 0;
    shared_stuff->total_timeA = 0;
    
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
    else{
        pthread_join( thread1, NULL);
    }
    
    // Print Stats
    printf("\nProcess B statitics\n");
    printf("Messages recieved: %d\n", shared_stuff->messages_recievedB);
    printf("Messages sent: %d\n", shared_stuff->messages_sentB);
    printf("Total number of packages recieved: %d\n", shared_stuff->total_packages_recievedB);
    printf("Total number of packages sent: %d\n", shared_stuff->total_packages_sentB);
    printf("Average number of packages recieved: %f\n",(float)shared_stuff->total_packages_recievedB/shared_stuff->messages_recievedB);
    printf("Average number of packages sent: %f\n", (float)shared_stuff->total_packages_sentB/shared_stuff->messages_sentB);
    printf("Average execution time of recieved packages: %d microseconds\n", shared_stuff->total_timeB/shared_stuff->messages_recievedB);

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
        shared_stuff->total_packages_recievedB ++;

        if (sem_post(&shared_stuff->sem_packet1) == -1){
            errExit("sem_post");
        }

        if(shared_stuff->first_packetA == 1){
            gettimeofday(&shared_stuff->endA, NULL);
            shared_stuff->total_timeB = shared_stuff->total_timeB + ((shared_stuff->endA.tv_sec * 1000000 + shared_stuff->endA.tv_usec)-(shared_stuff->startA.tv_sec * 1000000 + shared_stuff->startA.tv_usec));
        }
        
        if (strncmp(shared_stuff->some_textA, "#BYE#", 5) == 0){
            shared_stuff->total_packages_recievedB --;
            shared_stuff->running = 0;
        }

        if(shared_stuff->last_packetA == 1 && shared_stuff->running){
            printf("\33[2K\rA wrote: %s", local_buffer);
            memset(local_buffer, '\0', TEXT_SZ);
            memset(shared_stuff->some_textA, '\0', TEXT_SZ);
            shared_stuff->messages_recievedB ++;
        }
        
    }
    pthread_exit("temp_text");
}

void *produce(void *shared_s){
    struct shared_use_st *shared_stuff = shared_s;

    while(shared_stuff->running) {
        printf("Enter some text: ");
        fgets(shared_stuff->some_textB, TEXT_SZ, stdin);
        shared_stuff->last_packetB = 0;

        for(int i = 0; i < (int)strlen(shared_stuff->some_textB)-1; i=i+15){
            memset(shared_stuff->text_packetB, '\0', 15);   
            strncpy(shared_stuff->text_packetB, shared_stuff->some_textB + i, 15);
            shared_stuff->total_packages_sentB ++;

            if(i==0){
                shared_stuff->first_packetB = 1;
                gettimeofday(&shared_stuff->startB, NULL);
            }else{
                shared_stuff->first_packetB = 0;
            }
            
            if(i > (int)strlen(shared_stuff->some_textB)- 15){
                shared_stuff->last_packetB = 1;
            }

            /* Post 'sem2' to tell the peer that it can now access the modified data in shared memory. */
            if (sem_post(&shared_stuff->sem2) == -1){
                errExit("sem_post");
            }
            if (sem_wait(&shared_stuff->sem_packet2) == -1){
                errExit("sem_wait");
            }        
        }

        if (strncmp(shared_stuff->some_textB, "#BYE#", 5) == 0) {
            shared_stuff->running = 0;
            shared_stuff->cancelation = 1;
            shared_stuff->total_packages_recievedB --;
            shared_stuff->total_packages_sentB --;
            if (sem_post(&shared_stuff->sem1) == -1){
                errExit("sem_post");
            }
        }else{
            shared_stuff->messages_sentB ++;
        }
    }

    pthread_exit("temp_text");
}