#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define JOB_QUEUE_MAX_SIZE 10

/*******************
 *                 *
 *    Structures   *
 *                 *
 *******************/

typedef struct Job {
    char* job_name;
    int est_run_time;
    int priority;
    char* arg_list[];
} Job;

typedef struct Scheduler {
    char* policy;
    int queue_head;

    Job* job_cache;
} Scheduler;

typedef struct Dispatcher {
    int queue_tail;
} Dispatcher;

typedef struct JobQueue {
    Job* queue[JOB_QUEUE_MAX_SIZE];
    int queue_job_num;
} JobQueue;

/*****************************
 *                           *
 *    Function Prototypes    *
 *                           *
 *****************************/

void* schedulerModule();
void* dispatcherModule();
void displayGreeting();
void displayHelp();
void init();
void parseUserCommand(char* userCommand);
char* cleanCommand(char* cmd);

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
Job user_job;

int quit_flag;


/**********************
 *                    *
 *    Main Function   *
 *                    *
 **********************/

int main() {

    char* user_command;
    size_t user_command_size = 64; 
    user_command = (char*) malloc(user_command_size * sizeof(char));

    if(user_command == NULL) {
        fprintf(stderr, "Unable to malloc userCommand buffer\n");
        return 1;
    }

    init();

    pthread_t scheduler_thread, dispatcher_thread;

    int iret1 = pthread_create(&scheduler_thread, NULL, schedulerModule, NULL);
    int iret2 = pthread_create(&dispatcher_thread, NULL, dispatcherModule, NULL);

    displayGreeting();
    while(quit_flag == 0) {
        printf(">");
        getline(&user_command, &user_command_size, stdin);
        parseUserCommand(user_command);
    }

    pthread_join(scheduler_thread, NULL);
    pthread_join(dispatcher_thread, NULL);

    free(user_command);
    user_command = NULL;

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

    quit_flag = 0;

    scheduler.policy = "FCFS";
    scheduler.queue_head = 0;
    scheduler.job_cache = NULL;

    dispatcher.queue_tail = 0;

    int i;
    for(i=0; i<JOB_QUEUE_MAX_SIZE; i++) {
        job_queue.queue[i] = NULL;
    }

    job_queue.queue_job_num = 0;      
}

void* schedulerModule(void* ptr) {
    //printf("Scheduler is online and waiting...\n");

    while(quit_flag != 1) {
        pthread_mutex_lock(&condition_mutex);
        if(job_queue.queue_job_num >= JOB_QUEUE_MAX_SIZE) {
            pthread_cond_wait(&queue_condition, &condition_mutex);  
        }
        pthread_mutex_unlock(&condition_mutex);
        
        if(scheduler.job_cache != NULL) {

            printf("Scheduler detected job in pool\n");
            printf("\tScheduler head value before inserting job: %i\n", scheduler.queue_head);
            pthread_mutex_lock(&queue_mutex);
            printf("\tNumber of elements in queue before add: %i\n", job_queue.queue_job_num);
            job_queue.queue[scheduler.queue_head] = scheduler.job_cache;
            job_queue.queue_job_num++;
            printf("\tNumber of elements in queue after add: %i\n", job_queue.queue_job_num);
            pthread_mutex_unlock(&queue_mutex);
            scheduler.job_cache = NULL;

            if(scheduler.queue_head >= JOB_QUEUE_MAX_SIZE-1) {
                scheduler.queue_head = 0;
            }else {
                scheduler.queue_head++;
            }

            printf("\tScheduler head value after inserting job: %i\n", scheduler.queue_head);

            // signal to the dispatcher
            pthread_mutex_lock(&condition_mutex);
            pthread_cond_signal(&queue_condition);
            pthread_mutex_unlock(&condition_mutex);

        }         
    }

    printf("Scheduler Done\n");
 
}

void* dispatcherModule(void* ptr) {
    //printf("Dispatcher is online and waiting...\n");

    while(quit_flag != 1) {
        pthread_mutex_lock(&condition_mutex);
        if(job_queue.queue_job_num <= 0) {
            pthread_cond_wait(&queue_condition, &condition_mutex);
        }
        pthread_mutex_unlock(&condition_mutex);

        pthread_mutex_lock(&queue_mutex);     
        if(job_queue.queue[dispatcher.queue_tail] != NULL) {
            printf("Dispatcher detected job in queue\n");
            printf("\tDispatcher tail value before grabbing job: %i\n", dispatcher.queue_tail);
            Job* job = job_queue.queue[dispatcher.queue_tail];
            job_queue.queue[dispatcher.queue_tail] = NULL;
            job_queue.queue_job_num--;
            pthread_mutex_unlock(&queue_mutex);
            printf("\tJob was: %s\n", job->job_name); 
            
            if(dispatcher.queue_tail >= JOB_QUEUE_MAX_SIZE-1) {
                dispatcher.queue_tail = 0;
            } else {
                dispatcher.queue_tail++;
            }

            printf("\tDispatcher tail value after grabbing job: %i\n", dispatcher.queue_tail);

            pid_t pid;
            pid = fork();
  
            switch(pid) {
                case -1:
                    perror("fork");
                    break;
                case 0:
                    execv(job->job_name, job->arg_list);
                    break;
                default:
                    pid = wait(NULL);
                    break;
            }           
        }
    }

    printf("Dispatcher Done\n");

}

void displayGreeting() {
    printf("Welcome to Joshua Boyd's batch job scheduler Version 1.0\n");
    printf("Type 'help' to find more about AUbatch commands\n");
    printf("1. Submit new job\n");
    printf("2. Quit\n");
}

void displayHelp() {
    printf("\trun <job> <time> <pri>: submit a job named <job>,\n");
    printf("\t\t\t\texecution time is <time>,\n");
    printf("\t\t\t\tpriority time is <pri>\n\n");
    printf("\tlist: display the job status.\n\n");
    printf("\tfcfs: change the scheduling policy to FCFS.\n\n");
    printf("\tsjf: change the scheduling policy to SJF.\n\n");
    printf("\tpriority: change the scheduling policy to priority.\n\n");
    printf("\ttest <benchmark> <policy> <num_of_jobs> <priority_levels>\n\n");
    printf("\t\t<min_cpu_time> <max_cpu_time>\n\n");
    printf("\tquit: exit AUbatch\n\n");
}

void parseUserCommand(char* user_command) {

    // tokenize the command and start taking action based on the tokens 
    char* base_command = strtok(user_command, " ");  

    char* cleaned_command = cleanCommand(base_command);    

    if(strcmp(cleaned_command, "quit") == 0) {
        quit_flag = 1;
        // after setting the quit flag, broadcast to the other threads in case they
        // happen to be waiting still
        pthread_cond_broadcast(&queue_condition);
    } else if(strcmp(cleaned_command, "help") == 0) {
        displayHelp();
    } else if(strcmp(cleaned_command, "test") == 0) {
        user_job.job_name = "sampleProgram";
        user_job.arg_list[0] = user_job.job_name;
        user_job.est_run_time = 10;
        user_job.priority = 0;
        scheduler.job_cache = &user_job;
        printf("Job submitted\n");
    }else {
        printf("\tError: That command is not recognized. Type 'help' for a list of commands.\n\n");
    }

}

char* cleanCommand(char* cmd) {

    int i;
    
    int letter_count = 0;

    for(i=0; i<strlen(cmd); i++) {
        if(cmd[i] != ' ' && cmd[i] != '\n') {
            cmd[letter_count] = cmd[i];
            letter_count++;
        }       
    }

    cmd[letter_count] = '\0';

    return cmd;
}
