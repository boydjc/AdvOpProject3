#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define JOB_QUEUE_MAX_SIZE 10

/*******************
 *                 *
 *    Structures   *
 *                 *
 *******************/

typedef struct Scheduler {
    char* policy;
    int queue_head;

    /* This is a pool to hold jobs as they come until the scheduler
     * can get around to scheduling them */ 
    char job_cache;
} Scheduler;

typedef struct Dispatcher {
    int queue_tail;
} Dispatcher;

typedef struct JobQueue {
    char queue[JOB_QUEUE_MAX_SIZE];
    int queue_job_num;
} JobQueue;

/*****************************
 *                           *
 *    Function Prototypes    *
 *                           *
 *****************************/

void* schedulerModule();
void* dispatcherModule();
void displayMainMenu();
void init();

/***********************************************
 *                                             *
 *    Pthread condition and mutex variables    *
 *                                             *
 ***********************************************/

pthread_cond_t queue_condition = PTHREAD_COND_INITIALIZER;

pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

/*****************
 *               *
 *    Globals    *
 *               *
 *****************/

Dispatcher dispatcher;
Scheduler scheduler;
JobQueue job_queue;

int quitFlag;


/**********************
 *                    *
 *    Main Function   *
 *                    *
 **********************/

int main() {

    int userChoice = 0;

    init();

    pthread_t scheduler_thread, dispatcher_thread;

    int iret1 = pthread_create(&scheduler_thread, NULL, schedulerModule, NULL);
    int iret2 = pthread_create(&dispatcher_thread, NULL, dispatcherModule, NULL);

    printf("Hello from aubatch\n");
    while(quitFlag == 0) {
        displayMainMenu();
        scanf("%d", &userChoice);
        printf("User choice was: %d\n", userChoice);
        if(userChoice == 2) {
            quitFlag = 1;
            // after setting the quit flag, broadcast to the other threads in case they 
            // happen to be waiting still
            pthread_cond_broadcast(&queue_condition);
        } else {
            scheduler.job_cache = 'a';
            printf("Job submitted\n");
        }
    }

    pthread_join(scheduler_thread, NULL);
    pthread_join(dispatcher_thread, NULL);

    return 0;
}

/*****************************
 *                           *
 *    Function Definitions   *
 *                           *
 *                           *
 *****************************/

/*
 * This function gets things ready by setting a policy for the scheduler,
 * initializing the number of jobs in the queue, etc. */
void init() {

    quitFlag = 0;

    scheduler.policy = "FCFS";
    scheduler.queue_head = 0;
    scheduler.job_cache = '\0';

    dispatcher.queue_tail = 0;

    int i;
    for(i=0; i<JOB_QUEUE_MAX_SIZE; i++) {
        job_queue.queue[i] = '\0';
    }

    job_queue.queue_job_num = 0;      
}

void* schedulerModule(void* ptr) {
    printf("Scheduler is online and waiting...\n");

    while(quitFlag != 1) {
        pthread_mutex_lock(&condition_mutex);
        if(job_queue.queue_job_num >= JOB_QUEUE_MAX_SIZE) {
            pthread_cond_wait(&queue_condition, &condition_mutex);  
        }
        pthread_mutex_unlock(&condition_mutex);
        
        if(scheduler.job_cache != '\0') {

            printf("Scheduler detected job in pool\n");
            printf("\tScheduler head value: %i\n", scheduler.queue_head);
            pthread_mutex_lock(&queue_mutex);
            printf("\tNumber of elements in queue before add: %i\n", job_queue.queue_job_num);
            job_queue.queue[scheduler.queue_head] = scheduler.job_cache;
            job_queue.queue_job_num++;
            printf("\tNumber of elements in queue after add: %i\n", job_queue.queue_job_num);
            pthread_mutex_unlock(&queue_mutex);
            scheduler.job_cache = '\0';

            if(scheduler.queue_head >= JOB_QUEUE_MAX_SIZE) {
                scheduler.queue_head = 0;
            }else {
                scheduler.queue_head++;
            }

            // signal to the dispatcher
            pthread_mutex_lock(&condition_mutex);
            pthread_cond_signal(&queue_condition);
            pthread_mutex_unlock(&condition_mutex);

        }         
    }

    printf("Scheduler Done\n");
 
}

void* dispatcherModule(void* ptr) {
    printf("Dispatcher is online and waiting...\n");

    while(quitFlag != 1) {
        pthread_mutex_lock(&condition_mutex);
        if(job_queue.queue_job_num <= 0) {
            pthread_cond_wait(&queue_condition, &condition_mutex);
        }
        pthread_mutex_unlock(&condition_mutex);

        pthread_mutex_lock(&queue_mutex);     
        if(job_queue.queue[dispatcher.queue_tail] != '\0') {
            printf("Dispatcher detected job in queue\n");
            printf("\tDispatcher tail value: %i\n", dispatcher.queue_tail);
            char job = job_queue.queue[dispatcher.queue_tail];
            job_queue.queue[dispatcher.queue_tail] = '\0';
            job_queue.queue_job_num--;
            printf("\tJob was: %c\n", job);
            sleep(3); 
        }
        pthread_mutex_unlock(&queue_mutex);
        if(dispatcher.queue_tail >= JOB_QUEUE_MAX_SIZE) {
            dispatcher.queue_tail = 0;
        }else {
            dispatcher.queue_tail++;
        } 
    }

    printf("Dispatcher Done\n");

}

void displayMainMenu() {
    printf("1. Submit new job\n");
    printf("2. Quit\n");
    printf("> ");
}
