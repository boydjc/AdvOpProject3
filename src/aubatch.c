#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <menu.h>

#define JOB_QUEUE_MAX_SIZE 100
#define MAX_JOB_ARGS 20

/*******************
 *                 *
 *    Structures   *
 *                 *
 *******************/

typedef struct Job {
    char* job_name;
    int est_run_time;
    int priority;
    time_t unix_seconds;
    struct tm arrival_time;
    int is_running;
    char* arg_list[20];
} Job;

typedef struct Scheduler {
    char* policy;
    int expected_wait_time;
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
void reallocateJobQueue();

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
pthread_mutex_t dispatcher_mutex = PTHREAD_MUTEX_INITIALIZER; // for accessing dispatcher tail from main thread
pthread_mutex_t scheduler_mutex = PTHREAD_MUTEX_INITIALIZER; // for accessing expected wait time from dispatcher thread

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
int total_num_of_jobs;


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
    total_num_of_jobs = 0;

    scheduler.policy = "FCFS";
    scheduler.queue_head = 0;
    scheduler.expected_wait_time = 0;
 
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
            //printf("Scheduler inserting job\n");
            //printf("\tScheduler head value before inserting job: %i\n", scheduler.queue_head);
            pthread_mutex_lock(&queue_mutex);
            //printf("\tNumber of elements in queue before add: %i\n", job_queue.queue_job_num);
            job_queue.queue[scheduler.queue_head] = scheduler.job_cache;
            job_queue.queue_job_num++;
            //printf("\tNumber of elements in queue after add: %i\n", job_queue.queue_job_num);
            pthread_mutex_unlock(&queue_mutex);

            if(scheduler.queue_head >= JOB_QUEUE_MAX_SIZE-1) {
                scheduler.queue_head = 0;
            }else {
                scheduler.queue_head++;
            }

            //printf("\tScheduler head value after inserting job: %i\n", scheduler.queue_head);
            
            reallocateJobQueue();

            // signal to the dispatcher
            pthread_mutex_lock(&dispatcher_condition_mutex);
            pthread_cond_signal(&dispatcher_queue_condition);
            pthread_mutex_unlock(&dispatcher_condition_mutex);
        }         
    }

    printf("Waiting for executing job to be done...\n");
 
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
            //printf("Dispatcher detected job in queue\n");
            Job job = job_queue.queue[dispatcher.queue_tail];
            job_queue.queue[dispatcher.queue_tail].is_running = 1;
            pthread_mutex_unlock(&queue_mutex);

            pthread_mutex_lock(&dispatcher_mutex);
            //printf("\tDispatcher tail value before grabbing job: %i\n", dispatcher.queue_tail);
            if(dispatcher.queue_tail >= JOB_QUEUE_MAX_SIZE-1) {
                dispatcher.queue_tail = 0;
            } else {
                dispatcher.queue_tail++;
            }
            //printf("\tDispatcher tail value after grabbing job: %i\n", dispatcher.queue_tail);
            pthread_mutex_unlock(&dispatcher_mutex);

            if(job.job_name) {
                pid_t pid;
                pid = fork();
 
                switch(pid) {
                    case -1:
                        perror("fork");
                        break;
                    case 0:
                        execv(job.job_name, job.arg_list);
                        perror("execv");
                        //printf("\tJob was: %s\n", job.job_name);
                        exit(EXIT_FAILURE);
                        break;
                    default:
                        pid = wait(NULL);

                        pthread_mutex_lock(&queue_mutex);
                        job_queue.queue_job_num--;
                        pthread_mutex_unlock(&queue_mutex);

                        pthread_mutex_lock(&scheduler_mutex);
                        scheduler.expected_wait_time -= job.est_run_time;
                        pthread_mutex_unlock(&scheduler_mutex);
                        break;
                }

            }
        }           
    }

    printf("Goodbye\n");

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

        printf("Total number of job submitted: %i\n", total_num_of_jobs);
        printf("Average turnaround time: TODO\n");
        printf("Average CPU time: TODO\n");
        printf("Average waiting time: TODO\n");
        printf("Throughput: TODO\n");
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
            user_job.job_name = strdup(cleaned_command);
            user_job.arg_list[0] = strdup(cleaned_command);
            
            command = strtok(NULL, " ");
            if(command == NULL) {
                fprintf(stderr, "ERROR: You must specify an estimated runtime length for the given file.\n");
                displayRunHelp();
            } else {
                cleaned_command = cleanCommand(command);
                user_job.est_run_time = atoi(cleaned_command);
                command = strtok(NULL, " ");
                if(command == NULL) {
                    fprintf(stderr, "ERROR: You must specify a priority for the given file.\n");
                    displayRunHelp();
                } else {
                    cleaned_command = cleanCommand(command);
                    user_job.priority = atoi(cleaned_command);
                    user_job.unix_seconds = time(NULL);
                    localtime_r(&user_job.unix_seconds, &user_job.arrival_time);
                    user_job.is_running = 0;
                    
                    scheduler.job_cache = user_job;
                    total_num_of_jobs++;

                    // signal to scheduler and let it know we are ready for it to process the job
                    pthread_mutex_lock(&scheduler_condition_mutex);
                    pthread_cond_signal(&scheduler_queue_condition);
                    pthread_mutex_unlock(&scheduler_condition_mutex);

                    printf("Job %s was submitted\n", user_job.job_name);
                    pthread_mutex_lock(&queue_mutex);
                    printf("Total number of jobs in the queue: %i\n", job_queue.queue_job_num);
                    pthread_mutex_unlock(&queue_mutex);


                    pthread_mutex_lock(&scheduler_mutex);
                    scheduler.expected_wait_time += user_job.est_run_time;
                    printf("Expected waiting time: %i seconds\n", scheduler.expected_wait_time);
                    pthread_mutex_unlock(&scheduler_mutex);
                    printf("Scheduling Policy: %s\n", scheduler.policy);

                }
            }
        }
    } else if(strcmp(cleaned_command, "list") == 0) {
        pthread_mutex_lock(&queue_mutex);
        printf("Total number of jobs in the queue: %i\n", job_queue.queue_job_num);
        int job_count;
        int job_index;        

        pthread_mutex_lock(&dispatcher_mutex); 
        if(dispatcher.queue_tail == JOB_QUEUE_MAX_SIZE-1) {
            job_index = 0;
        } else {
            job_index = dispatcher.queue_tail-1;
        }
        pthread_mutex_unlock(&dispatcher_mutex);

        printf("Name\t\tCPU_TIME\tPRI\tArrival_time\tProgress\n");
        for(job_count=0; job_count<job_queue.queue_job_num; job_count++) {
            printf("%.8s\t%i\t\t%i\t%02d:%02d:%02d\t", job_queue.queue[job_index].job_name, 
                                                       job_queue.queue[job_index].est_run_time,
                                                       job_queue.queue[job_index].priority,
                                                       job_queue.queue[job_index].arrival_time.tm_hour,
                                                       job_queue.queue[job_index].arrival_time.tm_min,
                                                       job_queue.queue[job_index].arrival_time.tm_sec);

            if(job_queue.queue[job_index].is_running == 1) {
                printf("%s\n", "Running");
            } else {
                printf("\n");
            }

            if(job_index >= JOB_QUEUE_MAX_SIZE-1) {
                job_index = 0;
            } else {
                job_index++;
            }
        }
        pthread_mutex_unlock(&queue_mutex);
        printf("Scheduling Policy: %s\n", scheduler.policy);   
       
    } else if(strcmp(cleaned_command, "fcfs") == 0) {
        scheduler.policy = "FCFS";
        reallocateJobQueue();
        printf("Scheduling policy has been switched to FCFS.\n"); 
    } else if(strcmp(cleaned_command, "sjf") == 0) {
        scheduler.policy = "SJF";
        reallocateJobQueue();
        printf("Scheduling policy has been switched to SJF.\n");
    } else if(strcmp(cleaned_command, "priority") == 0) {
        scheduler.policy = "Priority";
        reallocateJobQueue();
        printf("Scheduling policy has been switched to Priority.\n");
    } else {
        printf("\tError: That command is not recognized. Type 'help' for a list of commands.\n\n");
    }

}


/* decided to use something like selection sort here where the minimum value will be 
 * dependent on what policy the scheduler is using. Doing it this 
 * way and having lower priority be more important will allow us to use
 * selection sort for each policy. */
void reallocateJobQueue() {

    Job new_queue[JOB_QUEUE_MAX_SIZE];

    int job_count = 0;
    int job_index, min_index;

    pthread_mutex_lock(&queue_mutex);
    
    if(job_queue.queue_job_num > 1) {

        //printf("Reallocating job queue\n");

        // make a deep copy of the job_queue
        memcpy(new_queue, job_queue.queue, sizeof(job_queue.queue));

        pthread_mutex_lock(&dispatcher_mutex);
        job_index = dispatcher.queue_tail;

        while(job_count < job_queue.queue_job_num-1) {

            min_index = job_index;
            
            int job_sub_index;
            if(job_index == JOB_QUEUE_MAX_SIZE-1) {
                if(job_queue.queue[0].is_running) {
                    job_sub_index = 1;
                } else {
                    job_sub_index = 0;
                }
            } else { 
                job_sub_index = job_index+1;
            }
            
            int job_sub_count = job_count+1;
            while(job_sub_count <= job_queue.queue_job_num-2) {

                //printf("Minimum\n");
                //printf("Job name %s\t Job Priority: %i\n", new_queue[min_index].job_name, new_queue[min_index].priority);
                //printf("Compared to\n");
                //printf("Job name: %s\t Job Priority: %i\n\n", new_queue[job_sub_index].job_name, new_queue[job_sub_index].priority);

                // here is where we check based on on different policies               
                pthread_mutex_lock(&scheduler_mutex);
                if(strcmp(scheduler.policy, "Priority") == 0) {
                    if(new_queue[job_sub_index].priority < new_queue[min_index].priority) {
                        min_index = job_sub_index;
                    }
                } else if(strcmp(scheduler.policy, "FCFS") == 0) {
                    if(new_queue[job_sub_index].unix_seconds < new_queue[min_index].unix_seconds) {
                        min_index = job_sub_index;
                    }
                } else if(strcmp(scheduler.policy, "SJF") == 0) {
                    if(new_queue[job_sub_index].est_run_time < new_queue[min_index].est_run_time) {
                        min_index = job_sub_index;
                    }
                }
                pthread_mutex_unlock(&scheduler_mutex);

                if(job_sub_index == JOB_QUEUE_MAX_SIZE-1) {
                    job_sub_index = 0;
                } else {
                    job_sub_index++;
                }

                job_sub_count++;

            }
             
            // swap min and current index
            Job temp_job = new_queue[min_index];
            /*printf("\tMinimum Job\n");
            printf("\tJob name: %s\t Job Priority: %i\n", temp_job.job_name, temp_job.priority);

            printf("\tSwapped with\n");
            printf("\tJob name: %s\t Job Priority: %i\n\n", new_queue[job_index].job_name, new_queue[job_index].priority);*/

            new_queue[min_index] = new_queue[job_index];
            new_queue[job_index] = temp_job; 

            if(job_index == JOB_QUEUE_MAX_SIZE-1) {
                job_index = 0;
            } else {
                job_index++;
            }


            job_count++;
        }         
        pthread_mutex_unlock(&dispatcher_mutex);
        memcpy(job_queue.queue, new_queue, sizeof(new_queue));
    }

    pthread_mutex_unlock(&queue_mutex);
    

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

