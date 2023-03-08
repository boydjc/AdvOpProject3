#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <menu.h>

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

    Job job_cache;
} Scheduler;

typedef struct Dispatcher {
    int queue_tail;
} Dispatcher;

typedef struct JobQueue {
    Job queue[JOB_QUEUE_MAX_SIZE];
    int queue_job_num;
} JobQueue;

/*****************************
 *                           *
 *    Function Prototypes    *
 *                           *
 *****************************/

void* schedulerModule();
void* dispatcherModule();
void init();
void parseUserCommand(char* userCommand);
char* cleanCommand(char* cmd);

/***********************************************
 *                                             *
 *    Pthread condition and mutex variables    *
 *                                             *
 ***********************************************/

pthread_cond_t scheduler_queue_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t dispatcher_queue_condition = PTHREAD_COND_INITIALIZER;

pthread_mutex_t scheduler_condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t dispatcher_condition_mutex = PTHREAD_MUTEX_INITIALIZER;

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
 
    dispatcher.queue_tail = 0;
    
    job_queue.queue_job_num = 0;      
}

void* schedulerModule(void* ptr) {
    //printf("Scheduler is online and waiting...\n");

    while(quit_flag != 1) {
        pthread_mutex_lock(&scheduler_condition_mutex);
        pthread_cond_wait(&scheduler_queue_condition, &scheduler_condition_mutex);  
        pthread_mutex_unlock(&scheduler_condition_mutex);
        
        int num_of_jobs = 0;
        pthread_mutex_lock(&queue_mutex);
        num_of_jobs = job_queue.queue_job_num;
        pthread_mutex_unlock(&queue_mutex);

        if(quit_flag != 1) {
            printf("Scheduler inserting job\n");
            printf("\tScheduler head value before inserting job: %i\n", scheduler.queue_head);
            pthread_mutex_lock(&queue_mutex);
            printf("\tNumber of elements in queue before add: %i\n", job_queue.queue_job_num);
            job_queue.queue[scheduler.queue_head] = scheduler.job_cache;
            job_queue.queue_job_num++;
            printf("\tNumber of elements in queue after add: %i\n", job_queue.queue_job_num);
            pthread_mutex_unlock(&queue_mutex);

            if(scheduler.queue_head >= JOB_QUEUE_MAX_SIZE-1) {
                scheduler.queue_head = 0;
            }else {
                scheduler.queue_head++;
            }

            printf("\tScheduler head value after inserting job: %i\n", scheduler.queue_head);

            // signal to the dispatcher
            pthread_mutex_lock(&dispatcher_condition_mutex);
            pthread_cond_signal(&dispatcher_queue_condition);
            pthread_mutex_unlock(&dispatcher_condition_mutex);
        }         
    }

    printf("Scheduler Done\n");
 
}

void* dispatcherModule(void* ptr) {
    //printf("Dispatcher is online and waiting...\n");

    while(quit_flag != 1) {
        pthread_mutex_lock(&dispatcher_condition_mutex);
        if(job_queue.queue_job_num <= 0) {
            pthread_cond_wait(&dispatcher_queue_condition, &dispatcher_condition_mutex);
        }
        pthread_mutex_unlock(&dispatcher_condition_mutex);
        
        int num_of_jobs = 0;
        pthread_mutex_lock(&queue_mutex);
        num_of_jobs = job_queue.queue_job_num;
        pthread_mutex_unlock(&queue_mutex);

        if(num_of_jobs != 0 && num_of_jobs < JOB_QUEUE_MAX_SIZE) {
            pthread_mutex_lock(&queue_mutex);
            printf("Dispatcher detected job in queue\n");
            Job job = job_queue.queue[dispatcher.queue_tail];
            pthread_mutex_unlock(&queue_mutex);

            job.arg_list[0] = "sampleProgram";
            job.arg_list[1] = NULL;

            if(job.job_name) {

                printf("\tDispatcher tail value before grabbing job: %i\n", dispatcher.queue_tail);
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
                        execv(job.job_name, job.arg_list);
                        perror("execv");
                        printf("\tJob was: %s\n", job.job_name);
                        exit(EXIT_FAILURE);
                        break;
                    default:
                        pid = wait(NULL);
                        pthread_mutex_lock(&queue_mutex);
                        job_queue.queue_job_num--;
                        pthread_mutex_unlock(&queue_mutex);
                        break;
                }

            }
        }           
    }

    printf("Dispatcher Done\n");

}

void parseUserCommand(char* user_command) {

    // tokenize the command and start taking action based on the tokens 
    char* command = strtok(user_command, " "); 

    char* cleaned_command = cleanCommand(command);    

    if(strcmp(cleaned_command, "quit") == 0) {
        quit_flag = 1;
        // after setting the quit flag, signal to both threads one more time in case they are waiting
        pthread_mutex_lock(&scheduler_condition_mutex);
        pthread_cond_signal(&scheduler_queue_condition);
        pthread_mutex_unlock(&scheduler_condition_mutex);
       
        pthread_mutex_lock(&dispatcher_condition_mutex);
        pthread_cond_signal(&dispatcher_queue_condition);
        pthread_mutex_unlock(&dispatcher_condition_mutex);
    } else if(strcmp(cleaned_command, "help") == 0) {
        command = strtok(NULL, " ");
        if(command == NULL) {
            displayHelp();
        } else {
            cleaned_command = cleanCommand(command);
            if(strcmp(cleaned_command, "-run") == 0){
                displayRunHelp();
            } else if(strcmp(cleaned_command, "-list") == 0) {
                displayListHelp();
            } else if(strcmp(cleaned_command, "-fcfs") == 0) {
                displayFcfsHelp();
            } else if(strcmp(cleaned_command, "-sjf") == 0) {
                displaySjfHelp();
            } else if(strcmp(cleaned_command, "-priority") == 0) {
                displayPriorityHelp();
            } else if(strcmp(cleaned_command, "-test") == 0) {
                displayTestHelp();
            } else if(strcmp(cleaned_command, "-quit") == 0) {
                displayQuitHelp();
            } else {
                fprintf(stderr, "Error: Command not found. Make sure you are passing the '-' char with your command you need help with.\n");
                fprintf(stderr, "Example: help -test\n");
            }
        }

    } else if(strcmp(cleaned_command, "run") == 0) {
        command = strtok(NULL, " ");
        if(command == NULL) {
            fprintf(stderr, "ERROR: You must specify a file to run.\n");
            displayRunHelp();
        } else {
            // program name
            cleaned_command = cleanCommand(command);
            scheduler.job_cache.job_name = cleaned_command;
            scheduler.job_cache.arg_list[0] = user_job.job_name;
            
            command = strtok(NULL, " ");
            if(command == NULL) {
                fprintf(stderr, "ERROR: You must specify an estimated runtime length for the given file.\n");
                displayRunHelp();
            } else {
                cleaned_command = cleanCommand(command);
                scheduler.job_cache.est_run_time = atoi(cleaned_command);
                scheduler.job_cache.arg_list[1] = NULL;
                command = strtok(NULL, " ");
                if(command == NULL) {
                    fprintf(stderr, "ERROR: You must specify a priority for the given file.\n");
                    displayRunHelp();
                } else {
                    cleaned_command = cleanCommand(command);
                    scheduler.job_cache.priority = atoi(cleaned_command);

                    printf("Job %s was submitted\n", scheduler.job_cache.job_name);
                    pthread_mutex_lock(&queue_mutex);
                    printf("Total number of jobs in the queue: %i\n", job_queue.queue_job_num);
                    pthread_mutex_unlock(&queue_mutex);
                    printf("Expected waiting time: !!!TODO!!!\n");
                    printf("Scheduling Policy: %s\n", scheduler.policy);
 
                    // signal to scheduler and let it know we are ready for it to process the job
                    pthread_mutex_lock(&scheduler_condition_mutex);
                    pthread_cond_signal(&scheduler_queue_condition);
                    pthread_mutex_unlock(&scheduler_condition_mutex);
                }
            }
        }
    } else if(strcmp(cleaned_command, "list") == 0) {
        pthread_mutex_lock(&queue_mutex);
        printf("Total number of jobs in the queue: %i\n", job_queue.queue_job_num);
        int job_count;
        printf("Name\t\tCPU_TIME\tPRI\tArrival_time\tProgress\n");
        for(job_count=0; job_count<job_queue.queue_job_num; job_count++) {
            printf("%.8s\t%i\t\t%i\t%s\t\t%s\n", job_queue.queue[job_count].job_name, 
                                                 job_queue.queue[job_count].est_run_time,
                                                 job_queue.queue[job_count].priority,
                                                 "TODO",
                                                 "TODO"); 
        }
        pthread_mutex_unlock(&queue_mutex);
        printf("Scheduling Policy: %s\n", scheduler.policy);
    } else if(strcmp(cleaned_command, "job") == 0) {
        scheduler.job_cache.job_name = "sampleProgram";
        scheduler.job_cache.arg_list[0] = user_job.job_name;
        scheduler.job_cache.arg_list[1] = NULL;
        scheduler.job_cache.priority = 2;
        scheduler.job_cache.est_run_time = 2;

        printf("Job %s was submitted.\n", scheduler.job_cache.job_name);
        printf("Job Est Run Time: %i\n", scheduler.job_cache.est_run_time);
        printf("Job Priority: %i\n", scheduler.job_cache.priority);        

        pthread_mutex_lock(&scheduler_condition_mutex);
        pthread_cond_signal(&scheduler_queue_condition);
        pthread_mutex_unlock(&scheduler_condition_mutex);
       
        
    } else if(strcmp(cleaned_command, "fcfs") == 0) {
        scheduler.policy = "FCFS";
        printf("Scheduling policy has been switched to FCFS.\n"); 
    } else if(strcmp(cleaned_command, "sjf") == 0) {
        scheduler.policy = "SJF";
        printf("Scheduling policy has been switched to SJF.\n");
    } else if(strcmp(cleaned_command, "priority") == 0) {
        scheduler.policy = "Priority";
        printf("Scheduling policy has been switched to Priority.\n");
    } else {
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

